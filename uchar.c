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

#include "compiler.h"
#include "sexp/uchar.h"

#include <stdio.h> // XXX: remove

const char hex_tab[16] = "0123456789abcdef";

/*
 * Byte Sequence                                             Min       Min        Max
 * ----------------------------------------------------------------------------------
 * 0xxxxxxx                                              0000000   0x00000   0x00007f
 * 110xxxxx 10xxxxxx                                000 10000000   0x00080   0x0007ff
 * 1110xxxx 10xxxxxx 10xxxxxx                  00001000 00000000   0x00800   0x00ffff
 * 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx   00001 00000000 00000000   0x10000   0x10ffff (not 0x1fffff)
 *
 * max: 100   001111   111111   111111  (0x10ffff)
 */

/* Length of UTF-8 byte sequence.
 * Table index is the first byte of UTF-8 sequence.
 */
static const signed char len_tab[256] = {
	/*   0-127  0xxxxxxx */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

	/* 128-191  10xxxxxx (invalid first byte) */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	/* 192-223  110xxxxx */
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,

	/* 224-239  1110xxxx */
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,

	/* 240-244  11110xxx (000 - 100) */
	4, 4, 4, 4, 4,

	/* 11110xxx (101 - 111) (always invalid) */
	-1, -1, -1,

	/* 11111xxx (always invalid) */
	-1, -1, -1, -1, -1, -1, -1, -1
};

/* fault-tolerant equivalent to len_tab, from glib */
static const char utf8_skip_data[256] = {
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
};
const char * const utf8_skip = utf8_skip_data;

/* index is length of the UTF-8 sequence - 1 */
static unsigned int min_val[4] = { 0x000000, 0x000080, 0x000800, 0x010000 };
static unsigned int max_val[4] = { 0x00007f, 0x0007ff, 0x00ffff, 0x10ffff };

/* get value bits from the first UTF-8 sequence byte */
static unsigned int first_byte_mask[4] = { 0x7f, 0x1f, 0x0f, 0x07 };

int u_is_valid(const char *str)
{
	const unsigned char *s = (const unsigned char *)str;
	int i = 0;

	while (s[i]) {
		unsigned char ch = s[i++];
		int len = len_tab[ch];

		if (len <= 0)
			return 0;

		if (len > 1) {
			/* len - 1 10xxxxxx bytes */
			uchar u;
			int c;

			len--;
			u = ch & first_byte_mask[len];
			c = len;
			do {
				ch = s[i++];
				if (len_tab[ch] != 0)
					return 0;
				u = (u << 6) | (ch & 0x3f);
			} while (--c);

			if (u < min_val[len] || u > max_val[len])
				return 0;
		}
	}
	return 1;
}

size_t u_strlen(const char *str)
{
	size_t len;
	for (len = 0; *str; len++)
		str = u_next_char(str);
	return len;
}

size_t u_strlen_safe(const char *str)
{
	const unsigned char *s = (const unsigned char *)str;
	size_t len = 0;

	while (*s) {
		int l = len_tab[*s];

		if (unlikely(l > 1)) {
			/* next l - 1 bytes must be 0x10xxxxxx */
			int c = 1;
			do {
				if (len_tab[s[c]] != 0) {
					/* invalid sequence */
					goto single_char;
				}
				c++;
			} while (c < l);

			/* valid sequence */
			s += l;
			len++;
			continue;
		}
single_char:
		/* l is -1, 0 or 1
		 * invalid chars counted as single characters */
		s++;
		len++;
	}
	return len;
}

uchar u_get_char(const char *str, int *idx)
{
	const unsigned char *s = (const unsigned char *)str;
	int len, i, x = 0;
	uchar ch, u;

	if (idx)
		s += *idx;
	else
		idx = &x;
	ch = s[0];

	/* ASCII optimization */
	if (ch < 128) {
		*idx += 1;
		return ch;
	}

	len = len_tab[ch];
	if (unlikely(len < 1))
		goto invalid;

	u = ch & first_byte_mask[len - 1];
	for (i = 1; i < len; i++) {
		ch = s[i];
		if (unlikely(len_tab[ch] != 0))
			goto invalid;
		u = (u << 6) | (ch & 0x3f);
	}
	*idx += len;
	return u;
invalid:
	*idx += 1;
	u = s[0];
	return u | U_INVALID_MASK;
}

void u_set_char_raw(char *str, int *idx, uchar uch)
{
	int i = *idx;

	if (uch <= 0x0000007fU) {
		str[i++] = uch;
		*idx = i;
	} else if (uch <= 0x000007ffU) {
		str[i + 1] = (uch & 63) | 0x80; uch >>= 6;
		str[i + 0] = uch | 0x000000c0U;
		i += 2;
		*idx = i;
	} else if (uch <= 0x0000ffffU) {
		str[i + 2] = (uch & 63) | 0x80; uch >>= 6;
		str[i + 1] = (uch & 63) | 0x80; uch >>= 6;
		str[i + 0] = uch | 0x000000e0U;
		i += 3;
		*idx = i;
	} else if (uch <= 0x0010ffffU) {
		str[i + 3] = (uch & 63) | 0x80; uch >>= 6;
		str[i + 2] = (uch & 63) | 0x80; uch >>= 6;
		str[i + 1] = (uch & 63) | 0x80; uch >>= 6;
		str[i + 0] = uch | 0x000000f0U;
		i += 4;
		*idx = i;
	} else {
		/* must be an invalid uchar */
		str[i++] = uch & 0xff;
		*idx = i;
	}
}

/*
 * Printing functions, these lose information
 */

void u_set_char(char *str, int *idx, uchar uch)
{
	int i = *idx;

	if (unlikely(uch <= 0x0000001fU))
		goto invalid;

	if (uch <= 0x0000007fU) {
		str[i++] = uch;
		*idx = i;
		return;
	} else if (uch <= 0x000007ffU) {
		str[i + 1] = (uch & 63) | 0x80; uch >>= 6;
		str[i + 0] = uch | 0x000000c0U;
		i += 2;
		*idx = i;
		return;
	} else if (uch <= 0x0000ffffU) {
		str[i + 2] = (uch & 63) | 0x80; uch >>= 6;
		str[i + 1] = (uch & 63) | 0x80; uch >>= 6;
		str[i + 0] = uch | 0x000000e0U;
		i += 3;
		*idx = i;
		return;
	} else if (uch <= 0x0010ffffU) {
		str[i + 3] = (uch & 63) | 0x80; uch >>= 6;
		str[i + 2] = (uch & 63) | 0x80; uch >>= 6;
		str[i + 1] = (uch & 63) | 0x80; uch >>= 6;
		str[i + 0] = uch | 0x000000f0U;
		i += 4;
		*idx = i;
		return;
	}
invalid:
	/* control character or invalid unicode */
	if (uch == 0) {
		/* handle this special case here to make the common case fast */
		str[i++] = uch;
		*idx = i;
	} else {
		str[i++] = '<';
		str[i++] = hex_tab[(uch >> 4) & 0xf];
		str[i++] = hex_tab[uch & 0xf];
		str[i++] = '>';
		*idx = i;
	}
}

