/* Public domain. */

#ifndef _LINUX_OVERFLOW_H
#define _LINUX_OVERFLOW_H

#define array_size(x, y)	((x) * (y))

#define struct_size(p, member, n) \
	(sizeof(*(p)) + ((n) * (sizeof(*(p)->member))))

#if defined(__clang__) || (defined(__GNUC__) && __GNUC__ >= 5)
#define check_add_overflow(x, y, sum)	__builtin_add_overflow(x, y, sum)
#define check_sub_overflow(x, y, z)	__builtin_sub_overflow(x, y, z)
#define check_mul_overflow(x, y, z)	__builtin_mul_overflow(x, y, z)
#else
#define check_mul_overflow(x, y, z) ({		\
	*(z) = (x) * (y);			\
	0;					\
})
#endif

static inline size_t
size_mul(size_t x, size_t y)
{
	size_t r;
	if (check_mul_overflow(x, y, &r))
		return SIZE_MAX;
	return r;
}

/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2021 Intel Corporation
 */

#define range_overflows(start, size, max) ({ \
	typeof(start) start__ = (start); \
	typeof(size) size__ = (size); \
	typeof(max) max__ = (max); \
	(void)(&start__ == &size__); \
	(void)(&start__ == &max__); \
	start__ >= max__ || size__ > max__ - start__; \
})

#endif
