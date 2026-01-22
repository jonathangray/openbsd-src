/* Public domain. */

#ifndef _LINUX_CGROUP_DMEM_H
#define _LINUX_CGROUP_DMEM_H

static inline void *
dmem_cgroup_register_region(uint64_t size, const char *fmt, ...)
{
	return NULL;
}

static inline void
dmem_cgroup_unregister_region(void *r)
{
}

static inline bool
dmem_cgroup_state_evict_valuable(void *a, void *b, bool c, bool *d)
{
	return true;
}

static inline void
dmem_cgroup_pool_state_put(void *a)
{
}

#endif
