/* Public domain. */

#ifndef _LINUX_INTERRUPT_H
#define _LINUX_INTERRUPT_H

#ifndef STUB
#include <sys/types.h>
#include <sys/systm.h>
#define STUB() do { printf("%s: stub\n", __func__); } while(0)
#endif

#define IRQF_SHARED	0

#define request_irq(irq, hdlr, flags, name, dev)	(0)
#define free_irq(irq, dev)

struct tasklet_struct {
	void (*func)(unsigned long);
	unsigned long data;
};

static inline void
tasklet_kill(struct tasklet_struct *ts)
{
	STUB();
}

static inline void
tasklet_schedule(struct tasklet_struct *ts)
{
	STUB();
}

static inline void
tasklet_hi_schedule(struct tasklet_struct *ts)
{
	STUB();
}

static inline void
tasklet_init(struct tasklet_struct *ts, void (*func)(unsigned long),
    unsigned long data)
{
	STUB();
}

static inline int
tasklet_trylock(struct tasklet_struct *ts)
{
	STUB();
	return 1;
}

static inline void
tasklet_unlock(struct tasklet_struct *ts)
{
	STUB();
}

#endif
