/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _NAVI_UNICODE_H
#define _NAVI_UNICODE_H

#include <assert.h>
#include <unicode/uchar.h>
#include <unicode/utf8.h>
#include <unicode/ucol.h>
#include <unicode/ucasemap.h>

/*
 * Wrappers for ICU macros.  The idea is here is to consolidate error checking
 * code and make it possible to turn error checking off for release builds.
 */

/* stop shouting! */
#define u8_length U8_LENGTH
#define u8_fwd_1(s, i, length) U8_FWD_1_UNSAFE(s, i)
#define u8_fwd_n(s, i, length, n) U8_FWD_N_UNSAFE(s, i, n)
#define u8_get_unchecked U8_GET
#define u8_next_unchecked U8_NEXT

#ifdef NDEBUG
/* use unsafe macros for release builds */
#define u8_append(s, i, capacity, c) U8_APPEND_UNSAFE(s, i, c)
#define u8_get(s, start, i, length, c) U8_GET_UNSAFE(s, i, c)
#define u8_next(s, i, length, c) U8_NEXT_UNSAFE(s, i, c)
#else
/* use safe macros with assertions for debug builds */
#define u8_append(s, i, capacity, c) \
	do { \
		UBool _u8_append_error = FALSE; \
		U8_APPEND(s, i, capacity, c, _u8_append_error); \
		assert(!_u8_append_error); \
	} while (0)
#define u8_get(s, start, i, length, c) \
	do { \
		U8_GET(s, start, i, length, c); \
		assert(c >= 0); \
	} while (0)
#define u8_next(s, i, length, c) \
	do { \
		U8_NEXT(s, i, length, c); \
		assert(c >= 0); \
	} while (0)
#endif /* NDEBUG */
#endif /* _NAVI_UNICODE_H */
