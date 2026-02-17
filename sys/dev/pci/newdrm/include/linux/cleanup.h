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

#define DEFINE_CLASS(_name, _type, _exit, _enter, _args...)	\
typedef _type class_##_name##_t;				\
static inline _type						\
class_##_name##_constructor(_args)				\
{								\
	_type t = _enter;					\
	return t;						\
}								\
static inline void						\
class_##_name##_destructor(_type *p)				\
{								\
	_type _T = *p;						\
	_exit;							\
}

#endif
