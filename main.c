// SPDX-License-Identifier: GPL-2.0
/*
 *  main.c
 *
 *  Google Translate scraper library usage example.
 *
 *  Copyright (C) 2021  Ammar Faizi
 */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cgtranslate.h"

int main(void)
{
	int ret;
	char curdir[128];
	char cache_dir[256];
	char cookie_dir[256];
	char *result = NULL;
	const char from[] = "en";
	const char to[] = "ja";
	const char text[] = "Good morning";
	cgtranslate_t *cg;

	/*
	 * Get current working directory (for cookie and cache)
	 */
	if (!getcwd(curdir, sizeof(curdir))) {
		ret = errno;
		printf("Error: getcwd(): %s\n", strerror(ret));
		return ret;
	}


	snprintf(cache_dir, sizeof(cache_dir), "%s/data/cache", curdir);
	snprintf(cookie_dir, sizeof(cookie_dir), "%s/data/cookie", curdir);

	cgtr_global_init();

	cg = cgtr_init();
	if (!cg) {
		ret = errno;
		printf("Error: cgtr_init(): %s\n", strerror(ret));
		return ret;
	}

	
	ret = cgtr_set_cache_dir(cg, cache_dir);
	if (ret != 0) {
		ret = -ret;
		printf("Error: cgtr_set_cache_dir(%s): %s\n", cache_dir,
			cgtr_get_err_str(cg));
		goto out_free;
	}


	ret = cgtr_set_cookie_dir(cg, cookie_dir);
	if (ret != 0) {
		ret = -ret;
		printf("Error: cgtr_set_cookie_dir(%s): %s\n", cookie_dir,
			cgtr_get_err_str(cg));
		goto out_free;
	}


	ret = cgtr_set_lang(cg, from, to);
	if (ret != 0) {
		ret = -ret;
		printf("Error: cgtr_set_lang(from=%s, to=%s): %s\n",
		       from, to, cgtr_get_err_str(cg));
		goto out_free;
	}


	ret = cgtr_set_text_ref(cg, text);
	if (ret != 0) {
		ret = -ret;
		printf("Error: cgtr_set_text(): %s\n", cgtr_get_err_str(cg));
		goto out_free;
	}

	ret = cgtr_execute(cg);
	if (ret != 0) {
		ret = -ret;
		printf("Error: cgtr_execute(): %s\n", cgtr_get_err_str(cg));
		goto out_free;
	}


	result = cgtr_detach_result(cg);
	if (!result) {
		printf("Got null pointer!\n");
		goto out_free;
	}

	printf("Source language: %s\n", from);
	printf("Target language: %s\n", to);
	printf("Text source: %s\n", text);
	printf("Translate result = %s\n", result);

out_free:
	free(result);
	cgtr_destroy(cg);
	cgtr_global_close();
	return ret;
}
