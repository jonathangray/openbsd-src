/* Public domain. */

#ifndef _LINUX_CLEANUP_H
#define _LINUX_CLEANUP_H

#define __free(fn)	__cleanup(__free_##fn)

/* function arg is effectively type *, deref is required */
#define DEFINE_FREE(fn, type, body)	\
static inline void			\
__free_##fn(void *p)			\
{					\
	type _T = *(type *)p;		\
	body;				\
}

#define no_free_ptr(p) \
	({void *_p = (p); (p) = NULL; _p; })

#endif
