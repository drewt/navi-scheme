/* Copyright 2014 Drew Thoreson
 * Copyright 2008-2013 Various Authors
 * Copyright 2005 Timo Hirvonen
 *
 * ((( Stolen largely from cmus )))
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

#ifndef _UCHAR_H
#define _UCHAR_H

#include <stddef.h> /* size_t */

typedef unsigned long uchar;

extern const char hex_tab[16];

/*
 * Invalid bytes are or'ed with this
 * for example 0xff -> 0x100000ff
 */
#define U_INVALID_MASK 0x10000000U

/*
 * @uch  potential unicode character
 *
 * Returns 1 if @uch is valid unicode character, 0 otherwise
 */
static inline int u_is_unicode(uchar uch)
{
	return uch <= 0x0010ffffU;
}

/*
 * Returns size of @uch in bytes
 */
static inline int u_char_size(uchar uch)
{
	if (uch <= 0x0000007fU) {
		return 1;
	} else if (uch <= 0x000007ffU) {
		return 2;
	} else if (uch <= 0x0000ffffU) {
		return 3;
	} else if (uch <= 0x0010ffffU) {
		return 4;
	} else {
		return 1;
	}
}

/*
 * @str  any null-terminated string
 *
 * Returns 1 if @str is valid UTF-8 string, 0 otherwise.
 */
int u_is_valid(const char *str, size_t start, size_t end);

/*
 * @str  valid, null-terminated UTF-8 string
 *
 * Returns position of next unicode character in @str.
 */
extern const char * const utf8_skip;
static inline char *u_next_char(const char *str)
{
	return (char *) (str + utf8_skip[*((const unsigned char *) str)]);
}

/*
 * @str  valid, null-terminated UTF-8 string
 *
 * Retuns length of @str in UTF-8 characters.
 */
size_t u_strlen(const char *str);

/*
 * @str  null-terminated UTF-8 string
 *
 * Retuns length of @str in UTF-8 characters.
 * Invalid chars are counted as single characters.
 */
size_t u_strlen_safe(const char *str);

/*
 * @str   UTF-8 string
 * @start character index at which to start counting
 * @end   character index at which to stop counting
 *
 * Returns the size of @str between @start and @end in bytes.
 */
size_t u_strsize(const char *str, size_t start, size_t end);

/*
 * @str  null-terminated UTF-8 string
 * @idx  pointer to byte index in @str (not UTF-8 character index!) or NULL
 *
 * Returns unicode character at @str[*@idx] or @str[0] if @idx is NULL.
 * Stores byte index of the next char back to @idx if set.
 */
uchar u_get_char(const char *str, size_t *idx);

/*
 * @str  destination buffer
 * @idx  pointer to byte index in @str (not UTF-8 character index!)
 * @uch  unicode character
 */
void u_set_char_raw(char *str, size_t *idx, uchar uch);
void u_set_char(char *str, size_t *idx, uchar uch);

static inline void u_skip_chars(const char *str, int n, size_t *idx)
{
	while (n-- > 0)
		u_get_char(str, idx);
}

#endif
