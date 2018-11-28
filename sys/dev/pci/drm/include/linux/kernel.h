/* Public domain. */

#ifndef _LINUX_KERNEL_H
#define _LINUX_KERNEL_H

#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/bitops.h>

#define swap(a, b) \
	do { __typeof(a) __tmp = (a); (a) = (b); (b) = __tmp; } while(0)

#define container_of(ptr, type, member) ({			\
	const __typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

#endif
