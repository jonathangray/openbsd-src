/* Public domain. */

#ifndef _LINUX_COMPILER_H
#define _LINUX_COMPILER_H

#define unlikely(x)	__builtin_expect(!!(x), 0)
#define likely(x)	__builtin_expect(!!(x), 1)

#endif
