/* Public domain. */

#ifndef _LINUX_FILE_H
#define _LINUX_FILE_H

/* both for printf */
#include <sys/types.h> 
#include <sys/systm.h>

#define fput(a)
#define fd_install(a, b)

int get_unused_fd_flags(unsigned int);
void put_unused_fd(int);

#endif
