/* Public domain. */

#ifndef _LINUX_COMPILER_H
#define _LINUX_COMPILER_H

#define unlikely(x)	__builtin_expect(!!(x), 0)
#define likely(x)	__builtin_expect(!!(x), 1)

#define __force
#define __always_unused __unused
#define __maybe_unused
#define __read_mostly
#define __iomem
#define __must_check
#define __init
#define __exit
#define __deprecated
#define __always_inline inline
#define noinline __attribute__((noinline))

#ifndef __user
#define __user
#endif

#define ACCESS_ONCE(x)		(*(volatile __typeof(x) *)&(x))

#define READ_ONCE(x) ({		\
	__typeof(x) __tmp = ({	\
		barrier();	\
		ACCESS_ONCE(x);	\
	});			\
	barrier();		\
	__tmp;			\
})

#define WRITE_ONCE(x, v) do {	\
	barrier();		\
	ACCESS_ONCE(x) = (v);	\
	barrier();		\
} while(0)

#define barrier()	__asm __volatile("" : : : "memory")

#endif
