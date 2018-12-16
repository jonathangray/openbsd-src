/* Public domain. */

#ifndef _LINUX_PROCESSOR_H
#define _LINUX_PROCESSOR_H

#include <sys/systm.h>
#include <machine/cpu.h>
#include <linux/jiffies.h>

static inline void
cpu_relax(void)
{
	CPU_BUSY_CYCLE();
	if (cold) {
		delay(tick);
		jiffies++;
	}
}

#endif
