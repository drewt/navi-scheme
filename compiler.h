/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _COMPILER_H
#define _COMPILER_H

#if __STDC_VERSION__ < 201112L
#define _Static_assert(cond, msg) \
	extern char navi_static_assert_fail[1/(cond)]
#ifdef __GNUC__
#define _Noreturn __attribute__((noreturn))
#define _Alignas(n) __attribute__((aligned(n)))
#else
#define _Noreturn
#define _Alignas(n)
#pragma message "*** WARNING: _Alignas defined as NOP ***"
#endif
#endif

/*
 * GCC 2.96 or compatible required
 */
#if defined(__GNUC__)
/* Optimization: Condition @x is likely */
#define likely(x) __builtin_expect(!!(x), 1)
/* Optimization: Condition @x is unlikely */
#define unlikely(x) __builtin_expect(!!(x), 0)

#define __used   __attribute__((used))
#define __unused __attribute__((unused))
#else
#define likely(x) (x)
#define unlikely(x) (x)

#define __used
#define __unused
#endif /* defined(__GNUC__) */
#endif /* _COMPILER_H */
