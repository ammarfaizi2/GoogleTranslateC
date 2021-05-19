// SPDX-License-Identifier: GPL-2.0
/*
 *  cgtranslate.c
 *
 *  Google Translate scraper library.
 *
 *  Copyright (C) 2021  Ammar Faizi
 *
 * https://github.com/ammarfaizi2/GoogleTranslateC
 */

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <pthread.h>
#include <stdbool.h>
#include <curl/curl.h>


#define CG_INTERNAL
#include "cgtranslate.h"


static bool global_init = false;
static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;


static char *my_strndup(const char *str, size_t max_len)
{
	char *ret;
	const char *orig = str;
	size_t len = 0;

	if (max_len == 0)
		return NULL;

	while (*str) {
		str++;
		if (++len >= max_len)
			break;
	}

	ret = malloc(len + 1);
	memcpy(ret, orig, len);
	ret[len] = '\0';
	return ret;
}


static size_t short_strlen(const char *str)
{
	size_t ret = 0;

	while (*str) {
		str++;
		ret++;
	}

	return ret;
}


void cgtr_global_init(void)
{
	pthread_mutex_lock(&init_mutex);
	if (!global_init) {
		curl_global_init(CURL_GLOBAL_ALL);
		global_init = true;
	}
	pthread_mutex_unlock(&init_mutex);
}


void cgtr_global_close(void)
{
	pthread_mutex_lock(&init_mutex);
	if (global_init) {
		curl_global_cleanup();
		global_init = false;
	}
	pthread_mutex_unlock(&init_mutex);
}


cgtranslate_t *cgtr_init()
{
	CURL *ch;
	cgtranslate_t *cg;

	cg = malloc(sizeof(*cg));
	if (!cg)
		return NULL;

	ch = curl_easy_init();
	if (!ch) {
		free(cg);
		errno = ENOMEM;
		return NULL;
	}

	cg->ch = ch;
	cg->cache_dir = NULL;
	cg->cookie_dir = NULL;
	cg->err_heap = NULL;
	cg->txt_heap = NULL;
	cg->res = NULL;
	cg->res_len = 0ul;
	cg->res_alloc = 0ul;
	cg->hdr_list = NULL;
	cg->clear_res = NULL;
	return cg;
}


int cgtr_set_cache_dir(cgtranslate_t *cg, const char *cache_dir)
{
	int ret;
	char *dir;

	ret = access(cache_dir, R_OK | W_OK);
	if (ret != F_OK) {
		ret = errno;
		cg->error_str = strerror(ret);
		return -ret;
	}

	dir = my_strndup(cache_dir, 512);
	if (!dir) {
		cg->error_str = strerror(ENOMEM);
		return -ENOMEM;
	}

	if (cg->cache_dir)
		free(cg->cache_dir);

	cg->cache_dir = dir;
	return 0;
}


int cgtr_set_cookie_dir(cgtranslate_t *cg, const char *cookie_dir)
{
	int ret;
	char *dir;

	ret = access(cookie_dir, R_OK | W_OK);
	if (ret != F_OK) {
		ret = errno;
		cg->error_str = strerror(ret);
		return -ret;
	}

	dir = my_strndup(cookie_dir, 512);
	if (!dir) {
		cg->error_str = strerror(ENOMEM);
		return -ENOMEM;
	}

	if (cg->cookie_dir)
		free(cg->cookie_dir);

	cg->cookie_dir = dir;
	return 0;
}


void cgtr_destroy(cgtranslate_t *cg)
{
	if (cg->hdr_list)
		curl_slist_free_all(cg->hdr_list);

	curl_easy_cleanup(cg->ch);
	free(cg->res);
	free(cg->clear_res);
	free(cg->txt_heap);
	free(cg->err_heap);
	free(cg->cookie_dir);
	free(cg->cache_dir);
	free(cg);
}


const char *cgtr_get_err_str(cgtranslate_t *cg)
{
	return cg->error_str;
}


int cgtr_set_lang(cgtranslate_t *cg, const char *from, const char *to)
{
	if (!to) {
		const char err_msg[] = "Target language cannot be NULL";
		free(cg->err_heap);
		cg->err_heap = my_strndup(err_msg, sizeof(err_msg));
		cg->error_str = cg->err_heap;
		return -EINVAL;
	}


	if (!from) {
		static_assert(sizeof("auto") <= sizeof(cg->from),
			      "Bad sizeof(cg->from)");

		memcpy(cg->from, "auto", sizeof("auto"));
	} else {
		strncpy(cg->from, from, sizeof(cg->from) - 1);
		cg->from[sizeof(cg->from) - 1] = '\0';
	}

	strncpy(cg->to, to, sizeof(cg->to) - 1);
	cg->to[sizeof(cg->to) - 1] = '\0';
	return 0;
}


int cgtr_set_text_ref_len(cgtranslate_t *cg, const char *text, size_t len)
{
	cg->text = text;
	cg->text_len = len;
	free(cg->txt_heap);
	cg->txt_heap = NULL;
	return 0;
}


int cgtr_set_text_ref(cgtranslate_t *cg, const char *text)
{
	return cgtr_set_text_ref_len(cg, text, strlen(text));
}


int cgtr_set_text_cpy_len(cgtranslate_t *cg, const char *text, size_t len)
{
	char *txt_heap = malloc(len + 1);
	if (!text) {
		cg->error_str = strerror(ENOMEM);
		return -ENOMEM;
	}

	memcpy(txt_heap, text, len);
	txt_heap[len] = '\0';

	free(cg->txt_heap);
	cg->txt_heap = txt_heap;
	cg->text = txt_heap;
	cg->text_len = len;
	return 0;
}


int cgtr_set_text_cpy(cgtranslate_t *cg, const char *text)
{
	return cgtr_set_text_cpy_len(cg, text, strlen(text));
}


/*
 * Thanks to PHP core.
 * https://github.com/php/php-src/blob/23961ef382e1005db6f8c08f3ecc0002839388a7/ext/standard/url.c#L459-L555
 */
char *urlencode(char *alloc, const char *s, size_t len, bool raw)
{
	static const unsigned char hexchars[] = "0123456789ABCDEF";
	register unsigned char c;
	unsigned char *to;
	unsigned char const *from, *end;
	char *start;

	from = (const unsigned char *)s;
	end = (const unsigned char *)s + len;

	if (alloc == NULL) {
		start = malloc((len * 3ul) + 1ul);
	} else {
		start = alloc;
	}

	to = (unsigned char *)start;

	while (from < end) {
		c = *from++;

		if (!raw && c == ' ') {
			*to++ = '+';
		} else if ((c < '0' && c != '-' && c != '.') ||
				(c < 'A' && c > '9') ||
				(c > 'Z' && c < 'a' && c != '_') ||
				(c > 'z' && (!raw || c != '~'))) {
			to[0] = '%';
			to[1] = hexchars[c >> 4];
			to[2] = hexchars[c & 15];
			to += 3;
		} else {
			*to++ = c;
		}
	}
	*to = '\0';

	return start;
}


static size_t write_callback(void *data, size_t size, size_t nmemb,
			     void *__restrict__ userp)
{
	cgtranslate_t *__restrict__ cg = userp;
	size_t add_len = size * nmemb;
	size_t new_len = cg->res_len + add_len;
	char *res = cg->res;

	if ((new_len + 1u) >= cg->res_alloc) {
		char *new_mem;

		cg->res_alloc = (cg->res_alloc * 2ul) + 1ul;
		new_mem = realloc(res, cg->res_alloc);
		if (!new_mem)
			return 0;

		res = cg->res = new_mem;
	}

	memcpy(res + cg->res_len, data, add_len);
	(res + cg->res_len)[add_len] = '\0';
	cg->res_len = new_len;

	return add_len;
}


static void build_url(cgtranslate_t *cg, char *url)
{
	memcpy(url, "https://translate.google.com/m?sl=", 34);
	url += 34;
	urlencode(url, cg->from, short_strlen(cg->from), false);
	while (*url)
		url++;

	memcpy(url, "&tl=", 4);
	url += 4;
	urlencode(url, cg->to, short_strlen(cg->to), false);
	while (*url)
		url++;

	memcpy(url, "&hl=en&q=", 9);
	url += 9;
	urlencode(url, cg->text, cg->text_len, false);
}


static int parse_response(cgtranslate_t *cg)
{
	static const char find[] = "<div class=\"result-container\">";
	char *p, *start;
	char *res = cg->res, *clear_res;
	size_t i = 0, j = 0;
	size_t max = cg->res_len;


	p = strstr(res, find);
	if (!p) {
		cg->error_str = "Cannot find translated result";
		return -EBADMSG;
	}

	p += sizeof(find) - 1;
	i = (size_t)(uintptr_t)(p - res);

	start = p;
	while (*p != '<') {
		p++;
		j++;
		if (i++ >= max) {
			cg->error_str = "Cannot find translated result";
			return -EBADMSG;
		}
	}
	*p = '\0';


	free(cg->clear_res);
	clear_res = malloc(j + 1u);
	if (!clear_res) {
		cg->error_str = strerror(ENOMEM);
		return -ENOMEM;
	}
	memcpy(clear_res, start, j + 1);
	cg->clear_res = clear_res;

	return 0;
}


static const char ua_header[] =
"User-Agent: Mozilla/5.0 (S60; SymbOS; Opera Mobi/SYB-1103211396; U; es-LA; rv:1.9.1.6) Gecko/20091201 Firefox/3.5.6 Opera 11.00";


int cgtr_execute(cgtranslate_t *cg)
{
	char *url;
	int ret = 0;
	CURLcode cres;
	CURL *ch = cg->ch;
	char cookie_file[512];
	struct curl_slist *list = cg->hdr_list;

	if (cg->res == NULL) {
		cg->res_alloc = 8192ul;
		cg->res = malloc(cg->res_alloc);
		if (!cg->res) {
			cg->error_str = strerror(ENOMEM);
			return -ENOMEM;
		}
	}
	cg->res_len = 0ul;

	url = malloc(cg->text_len * 3ul + 512ul);
	if (!url) {
		cg->error_str = strerror(ENOMEM);
		return -ENOMEM;
	}
	build_url(cg, url);

	if (!list) {
		list = curl_slist_append(list, ua_header);
		cg->hdr_list = list;
	}

	snprintf(cookie_file, sizeof(cookie_file), "%s/cgtranslate.cookie",
		 cg->cookie_dir);

	curl_easy_setopt(ch, CURLOPT_URL, url);
	curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(ch, CURLOPT_WRITEDATA, cg);
	curl_easy_setopt(ch, CURLOPT_HTTPHEADER, list);
	curl_easy_setopt(ch, CURLOPT_COOKIEJAR, cookie_file);
	curl_easy_setopt(ch, CURLOPT_COOKIEFILE, cookie_file);
	cres = curl_easy_perform(ch);
	if (cres != CURLE_OK) {
		cg->error_str = curl_easy_strerror(cres);
		ret = -EBADMSG;
		goto out;
	}

	ret = parse_response(cg);
out:
	free(url);
	return ret;
}


const char *cgtr_get_result(cgtranslate_t *cg)
{
	return cg->clear_res;
}


char *cgtr_detach_result(cgtranslate_t *cg)
{
	char *clear_res = cg->clear_res;
	cg->clear_res = NULL;
	return clear_res;
}
