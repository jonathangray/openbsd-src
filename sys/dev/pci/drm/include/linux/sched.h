/*	$OpenBSD$	*/
/*
 * Copyright (c) 2013, 2014, 2015 Mark Kettenis
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _LINUX_SCHED_H
#define _LINUX_SCHED_H

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/stdint.h>
#include <sys/mutex.h>
#include <linux/wait.h>

#define TASK_NORMAL		1
#define TASK_UNINTERRUPTIBLE	0
#define TASK_INTERRUPTIBLE	PCATCH
#define TASK_RUNNING		-1

#define MAX_SCHEDULE_TIMEOUT (INT32_MAX)

#define cond_resched()		sched_pause(yield)
#define drm_need_resched() \
    (curcpu()->ci_schedstate.spc_schedflags & SPCF_SHOULDYIELD)

static inline void
set_current_state(int state)
{
	if (sch_ident != curproc)
		mtx_enter(&sch_mtx);
	MUTEX_ASSERT_LOCKED(&sch_mtx);
	sch_ident = curproc;
	sch_priority = state;
}

static inline void
__set_current_state(int state)
{
	KASSERT(state == TASK_RUNNING);
	if (sch_ident == curproc) {
		MUTEX_ASSERT_LOCKED(&sch_mtx);
		sch_ident = NULL;
		mtx_leave(&sch_mtx);
	}
}

static inline long
schedule_timeout(long timeout)
{
	int err;
	long deadline;

	if (cold) {
		delay((timeout * 1000000) / hz);
		return 0;
	}

	if (timeout == MAX_SCHEDULE_TIMEOUT) {
		err = msleep(sch_ident, &sch_mtx, sch_priority, "schto", 0);
		sch_ident = curproc;
		return timeout;
	}

	deadline = ticks + timeout;
	err = msleep(sch_ident, &sch_mtx, sch_priority, "schto", timeout);
	timeout = deadline - ticks;
	if (timeout < 0)
		timeout = 0;
	sch_ident = curproc;
	return timeout;
}

#define io_schedule_timeout(x)	schedule_timeout(x)

static inline void
schedule(void)
{
	KASSERT(!cold);
	msleep(sch_ident, &sch_mtx, sch_priority, "schto", 0);
	sch_ident = curproc;
}

#endif
