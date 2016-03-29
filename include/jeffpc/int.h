/*
 * Copyright (c) 2016 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __JEFFPC_INT_H
#define __JEFFPC_INT_H

#include <inttypes.h>
#include <sys/byteorder.h>

/*
 * string to integer conversion
 */
#define STR_TO_INT(size, imax)						\
static inline int __str2u##size(const char *restrict s,			\
				uint##size##_t *i,			\
				int base)				\
{									\
	char *endptr;							\
	uint64_t tmp;							\
									\
	*i = 0;								\
									\
	errno = 0;							\
	tmp = strtoull(s, &endptr, base);				\
									\
	if (errno)							\
		return -errno;						\
									\
	if (endptr == s)						\
		return -EINVAL;						\
									\
	if (tmp > imax)							\
		return -ERANGE;						\
									\
	*i = tmp;							\
	return 0;							\
}

STR_TO_INT(16, 0x000000000000fffful)
STR_TO_INT(32, 0x00000000fffffffful)
STR_TO_INT(64, 0xffffffffffffffffull)

#undef STR_TO_INT

#define str2u64(s, i)	__str2u64((s), (i), 10)
#define str2u32(s, i)	__str2u32((s), (i), 10)
#define str2u16(s, i)	__str2u16((s), (i), 10)

/*
 * byte ordering
 */
#define __GEN(from, size, to, bswap)					\
static inline uint##size##_t from##size##_to_##to(uint##size##_t x)	\
{									\
	return bswap;							\
}

#ifdef CPU_BIG_ENDIAN
#define GEN(size)							\
	__GEN(be,  size, cpu, x)					\
	__GEN(cpu, size, be,  x)					\
	__GEN(le,  size, cpu, BSWAP_##size(x))				\
	__GEN(cpu, size, le,  BSWAP_##size(x))
#else
#define GEN(size)							\
	__GEN(be,  size, cpu, BSWAP_##size(x))				\
	__GEN(cpu, size, be,  BSWAP_##size(x))				\
	__GEN(le,  size, cpu, x)					\
	__GEN(cpu, size, le,  x)
#endif

GEN(64)
GEN(32)
GEN(16)
GEN(8)

#undef __GEN
#undef GEN

#endif
