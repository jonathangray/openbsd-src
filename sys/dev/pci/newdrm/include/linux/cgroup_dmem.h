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

#endif
