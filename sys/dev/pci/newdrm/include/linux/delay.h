/* Public domain. */

#ifndef _LINUX_DELAY_H
#define _LINUX_DELAY_H

#include <sys/param.h>
#include <sys/systm.h>

static inline void
udelay(unsigned long usecs)
{
	DELAY(usecs);
}

static inline void
ndelay(unsigned long nsecs)
{
	DELAY(MAX(nsecs / 1000, 1));
}

static inline void
usleep_range(unsigned long min, unsigned long max)
{
	tsleep_nsec(&min, PWAIT, "usleepr", USEC_TO_NSEC(((min + max) / 2)));
}

static inline void
mdelay(unsigned long msecs)
{
	int loops = msecs;
	while (loops--)
		DELAY(1000);
}

static inline void
drm_msleep(unsigned int msecs)
{
	tsleep_nsec(&msecs, PWAIT, "drmsleep", MSEC_TO_NSEC(msecs));
}

#endif
