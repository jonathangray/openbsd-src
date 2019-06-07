/* Public domain. */

#ifndef _LINUX_KTHREAD_H
#define _LINUX_KTHREAD_H

/* both for printf */
#include <sys/types.h>
#include <sys/systm.h>

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/wait.h>

#include <linux/sched.h>

static inline void
kthread_parkme(void)
{
	while (tsleep(curproc->p_p, PPAUSE|PCATCH, "park", 0) == 0)
		;
}

static inline bool
kthread_should_park(void)
{
	return (curproc->p_p->ps_flags & PS_SHOULDPARK);
}

static inline int
kthread_park(struct proc *p)
{
	atomic_setbits_int(&p->p_p->ps_flags, PS_SHOULDPARK);
	if (p != curproc)
		wakeup(p->p_p);
	return 0;
}

static inline void
kthread_unpark(struct proc *p)
{
	atomic_clearbits_int(&p->p_p->ps_flags, PS_SHOULDPARK);
	if (p != curproc)
		wakeup(p->p_p);
}

static inline bool
kthread_should_stop(void)
{
	return (curproc->p_p->ps_flags & PS_SHOULDSTOP);
}

static inline int
kthread_stop(struct proc *p)
{
	atomic_setbits_int(&p->p_p->ps_flags, PS_SHOULDSTOP);
	wakeup(p->p_p);
	return 0;
}

#endif
