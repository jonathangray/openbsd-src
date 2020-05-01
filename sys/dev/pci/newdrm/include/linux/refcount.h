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

#endif
