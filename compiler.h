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

#ifndef _COMPILER_H
#define _COMPILER_H

/*
 * GCC 2.96 or compatible required
 */
#if defined(__GNUC__)

#if __GNUC__ > 3
#undef offsetof
#define offsetof(type, member) __builtin_offsetof(type, member)
#endif

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
