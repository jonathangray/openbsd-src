/*
 * adapted from libkern/ffs.c
 * sparc64 does not have __ctzdi2 gcc emits for __builtin_ctzl
 */

/*
 * Public domain.
 * Written by Dale Rahn.
 */

#include <lib/libkern/libkern.h>

/*
 * ffsl -- vax ffs instruction with long arg
 */
static inline int
ffsl(long mask)
{
	int bit;
	unsigned long r = mask;
#ifdef __LP64__
	static const signed char t[16] = {
		-60, 1, 2, 1,
		  3, 1, 2, 1,
		  4, 1, 2, 1,
		  3, 1, 2, 1
	};
#else
	static const signed char t[16] = {
		-28, 1, 2, 1,
		  3, 1, 2, 1,
		  4, 1, 2, 1,
		  3, 1, 2, 1
	};
#endif

	bit = 0;
#ifdef __LP64__
	if (!(r & 0xffffffff)) {
		bit += 32;
		r >>= 32;
	}
#endif
	if (!(r & 0xffff)) {
		bit += 16;
		r >>= 16;
	}
	if (!(r & 0xff)) {
		bit += 8;
		r >>= 8;
	}
	if (!(r & 0xf)) {
		bit += 4;
		r >>= 4;
	}

	return (bit + t[ r & 0xf ]);
}

int
ctzl(long mask)
{
	if (mask == 0)
#ifdef __LP64__
		return 64;
#else
		return 32;
#endif
	return (ffsl(mask) - 1);
}
