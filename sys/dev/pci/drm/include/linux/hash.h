/* Public domain. */

#ifndef _LINUX_HASH_H
#define _LINUX_HASH_H

#include <sys/types.h>

/* 2^32 * ((sqrt(5) - 1) / 2) from Knuth */
#define GOLDEN_RATIO_32 0x9e3779b9

static inline uint32_t
hash_32(uint32_t val, unsigned int bits)
{
	return (val * GOLDEN_RATIO_32) >> (32 - bits);
}

#endif
