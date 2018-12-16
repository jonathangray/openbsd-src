/* Public domain. */

#ifndef _LINUX_INTERRUPT_H
#define _LINUX_INTERRUPT_H

#include <machine/intr.h>
#include <linux/hardirq.h>
#include <linux/irqflags.h>
#include <linux/atomic.h>

#ifndef STUB
#include <sys/types.h>
#include <sys/systm.h>
#define STUB() do { printf("%s: stub\n", __func__); } while(0)
#endif

#define IRQF_SHARED	0

#define disable_irq(x)		intr_disable()
#define enable_irq(x)		intr_enable()

#define request_irq(irq, hdlr, flags, name, dev)	(0)
#define free_irq(irq, dev)

struct tasklet_struct {
	void (*func)(unsigned long);
	unsigned long data;
	atomic_t count;
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
	ts->func = func;
	ts->data = data;
	atomic_set(&ts->count, 0);
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

static inline void
tasklet_unlock_wait(struct tasklet_struct *ts)
{
	STUB();
}

#endif
