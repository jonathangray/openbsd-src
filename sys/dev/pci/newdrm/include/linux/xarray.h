/* Public domain. */

#ifndef _LINUX_XARRAY_H
#define _LINUX_XARRAY_H

#include <linux/gfp.h>

#define XA_FLAGS_ALLOC		1
#define XA_FLAGS_ALLOC1		2

struct xarray {
};

static inline void
xa_init_flags(struct xarray *xa, gfp_t flags)
{
	STUB();
}

static inline void
xa_destroy(struct xarray *xa)
{
	STUB();
}

#define xa_limit_32b	0

static inline int
xa_alloc(struct xarray *xa, u32 *id, void *entry, int limit, gfp_t gfp)
{
	STUB();
	*id = 0;
	return 0;
}

static inline void *
xa_mk_value(unsigned long v)
{
	unsigned long r = (v << 1) | 1;
	return (void *)r;
}

static inline bool
xa_is_value(const void *e)
{
	unsigned long v = (unsigned long)e;
	return v & 1;
}

static inline unsigned long
xa_to_value(const void *e)
{
	unsigned long v = (unsigned long)e;
	return v >> 1;
}

#endif
