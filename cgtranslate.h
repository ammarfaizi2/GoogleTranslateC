// SPDX-License-Identifier: GPL-2.0
/*
 *  cgtranslate.h
 *
 *  Google Translate scraper library.
 *
 *  Copyright (C) 2021  Ammar Faizi
 *
 * https://github.com/ammarfaizi2/GoogleTranslateC
 */

#ifndef CGTRANSLATE_H
#define CGTRANSLATE_H

#include <curl/curl.h>


/*
 * User space is not allowed to edit
 * the struct by hand, so we add const here.
 */
#ifndef CG_INTERNAL
#  define UCONST const
#else
#  define UCONST
#endif


typedef struct _cgtranslate_t {
	UCONST CURL			*UCONST ch;
	UCONST char			to[8];
	UCONST char			from[8];
	UCONST size_t			text_len;
	const char			*UCONST text;
	UCONST char			*UCONST txt_heap;
	UCONST char			*UCONST cache_dir;
	UCONST char			*UCONST cookie_dir;
	const char			*UCONST error_str;
	UCONST char			*UCONST err_heap;

	UCONST char			*UCONST res;
	UCONST size_t			res_len;
	UCONST size_t			res_alloc;
	UCONST char			*clear_res;

	UCONST struct curl_slist	*UCONST hdr_list;
} cgtranslate_t;


void cgtr_global_init(void);
void cgtr_global_close(void);
cgtranslate_t *cgtr_init();
void cgtr_destroy(cgtranslate_t *cg);
int cgtr_set_cookie_dir(cgtranslate_t *cg, const char *cookie_dir);
int cgtr_set_cache_dir(cgtranslate_t *cg, const char *cache_dir);
const char *cgtr_get_err_str(cgtranslate_t *cg);
int cgtr_set_lang(cgtranslate_t *cg, const char *from, const char *to);

int cgtr_set_text_ref(cgtranslate_t *cg, const char *text);
int cgtr_set_text_ref_len(cgtranslate_t *cg, const char *text, size_t len);

int cgtr_set_text_cpy(cgtranslate_t *cg, const char *text);
int cgtr_set_text_cpy_len(cgtranslate_t *cg, const char *text, size_t len);

int cgtr_execute(cgtranslate_t *cg);
const char *cgtr_get_result(cgtranslate_t *cg);
char *cgtr_detach_result(cgtranslate_t *cg);

#endif /* #ifndef CGTRANSLATE_H */
