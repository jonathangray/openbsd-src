/* Public domain. */

#ifndef _LINUX_JIFFIES_H
#define _LINUX_JIFFIES_H

#include <sys/time.h>
#include <sys/limits.h>

extern volatile unsigned long jiffies;
#define jiffies_64 jiffies /* XXX */
#undef HZ
#define HZ	hz

#define MAX_JIFFY_OFFSET	((INT_MAX >> 1) - 1)

#endif
