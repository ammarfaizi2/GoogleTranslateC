#
# SPDX-License-Identifier: GPL-2.0
#
# @author Ammar Faizi <ammarfaizi2@gmail.com> https://www.facebook.com/ammarfaizi2
# @license GNU GPL-2.0
# @link https://github.com/ammarfaizi2/GoogleTranslateC
#
# Google Translate scraper library.
#
# Copyright (C) 2021  Ammar Faizi
#

CC 	:= clang
LD	:= $(CC)
VG	:= valgrind
CFLAGS	:= -Wall -Wextra -pedantic-errors -std=c11 -O3 -ggdb3
VGFLAGS	:= --leak-check=full --show-leak-kinds=all --track-origins=yes
LD_SHARED_LIB	:= -lcurl -lpthread

export LD_LIBRARY_PATH=$(PWD)

all: main


main: main.c libcgtranslate.so
	$(CC) $(CFLAGS) -fpie -fPIE $(^) -o $(@)

libcgtranslate.so: cgtranslate.c cgtranslate.h
	$(CC) $(CFLAGS) -fpic -fPIC $(<) -shared -o $(@) $(LD_SHARED_LIB)


clean:
	$(RM) -vf libcgtranslate.so main


run: main
	$(VG) $(VGFLAGS) ./$(<)

.PHONY: all clean run
