/* Copyright 2014-2015 Drew Thoreson
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef _NAVI_UNICODE_H
#define _NAVI_UNICODE_H

#include <assert.h>
#include <unicode/uchar.h>
#include <unicode/utf8.h>
#include <unicode/ucol.h>

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
