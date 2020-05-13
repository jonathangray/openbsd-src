/* Public domain. */

#ifndef _LINUX_REFCOUNT_H
#define _LINUX_REFCOUNT_H

#include <sys/types.h>
#include <linux/atomic.h>

typedef atomic_t refcount_t;

static inline bool
refcount_dec_and_test(uint32_t *p)
{
	return atomic_dec_and_test(p);
}

static inline bool
refcount_inc_not_zero(uint32_t *p)
{
	return atomic_inc_not_zero(p);
}

static inline void
refcount_set(uint32_t *p, int v)
{
	atomic_set(p, v);
}

#endif
