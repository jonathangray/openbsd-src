/*-
 * Copyright (c) 2003-2011 The DragonFly Project.  All rights reserved.
 * Copyright (c) 2015 Michael Neumann <mneumann@ntecs.de>.  All rights reserved.
 *
 * This code is derived from software contributed to The DragonFly Project
 * by Matthew Dillon <dillon@backplane.com> and
 *    Michael Neumann <mneumann@ntecs.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/proc.h>

#include <machine/atomic.h>

#include <linux/ww_mutex.h>

void
ww_acquire_init(struct ww_acquire_ctx *ctx, struct ww_class *ww_class)
{
	ctx->stamp = __sync_fetch_and_add(&ww_class->stamp, 1);
	ctx->acquired = 0;
	ctx->ww_class = ww_class;
}

void
ww_acquire_done(struct ww_acquire_ctx *ctx __unused)
{
}

void
ww_acquire_fini(struct ww_acquire_ctx *ctx __unused)
{
}

void
ww_mutex_init(struct ww_mutex *ww, struct ww_class *ww_class)
{
	rrw_init(&ww->base, ww_class->name);
	ww->ctx = NULL;
	ww->stamp = 0xFFFFFFFFFFFFFFFFLU;
	ww->blocked = 0;
}

void
ww_mutex_destroy(struct ww_mutex *ww)
{
}

/*
 * Optimized lock path.
 *
 * (slow) is optional as long as we block normally on the initial lock.
 * Currently not implemented.
 */
static __inline
int
__wwlock(struct ww_mutex *ww, struct ww_acquire_ctx *ctx,
	 bool slow __unused, bool intr)
{
	struct sleep_state sls;
	int flags = RW_WRITE;
	int error;

	if (intr)
		flags |= RW_INTR;

	/*
	 * Normal mutex if ctx is NULL
	 */
	if (ctx == NULL) {
		error = rrw_enter(&ww->base, flags);
		if (error)
			error = -EINTR;
		return error;
	}

	/*
	 * A normal blocking lock can be used when ctx->acquired is 0 (no
	 * prior locks are held).  If prior locks are held then we cannot
	 * block here.
	 *
	 * In the non-blocking case setup our tsleep interlock prior to
	 * attempting to acquire the lock.
	 */
	for (;;) {
		if (ctx->acquired != 0) {
			flags |= RW_NOSLEEP;
			sleep_setup(&sls, ww, (intr ? PCATCH : 0),
			    ctx->ww_class->name);
			sleep_setup_timeout(&sls, 0);
			sleep_setup_signal(&sls, (intr ? PCATCH : 0));
		}
		error = rrw_enter(&ww->base, flags);
		if (error == 0) {
			ww->ctx = ctx;
			ww->stamp = ctx->stamp;
			++ctx->acquired;
			return 0;
		}

		/*
		 * EINTR or ERESTART returns -EINTR.  ENOLCK and EWOULDBLOCK
		 * cannot happen (LK_SLEEPFAIL not set, timeout is not set).
		 */
		if (error != EBUSY)
			return -EINTR;

		/*
		 * acquired can only be non-zero in this path.
		 * NOTE: ww->ctx is not MPSAFE.
		 * NOTE: ww->stamp is heuristical, a race is possible.
		 */
		KASSERT(ctx->acquired > 0);

		/*
		 * Unwind if we aren't the oldest.
		 */
		if (ctx->stamp > ww->stamp)
			return -EDEADLK;

		/*
		 * We have priority over the currently held lock.  Tell
		 * the remote lock holder that we want them to unwind.
		 *
		 * error is zero if woken up
		 *	    EINTR / ERESTART - signal
		 *	    EWOULDBLOCK	     - timeout expired (if not 0)
		 */
		__sync_lock_test_and_set(&ww->blocked, 1);
		error = sleep_finish_all(&sls, 1);
		if (intr && (error == EINTR || error == ERESTART))
			return -EINTR;
		/* retry */
	}
}

int
ww_mutex_lock(struct ww_mutex *ww, struct ww_acquire_ctx *ctx)
{
	return __wwlock(ww, ctx, 0, 0);
}

int
ww_mutex_lock_slow(struct ww_mutex *ww, struct ww_acquire_ctx *ctx)
{
	return __wwlock(ww, ctx, 1, 0);
}

int
ww_mutex_lock_interruptible(struct ww_mutex *ww, struct ww_acquire_ctx *ctx)
{
	return __wwlock(ww, ctx, 0, 1);
}

int
ww_mutex_lock_slow_interruptible(struct ww_mutex *ww,
				 struct ww_acquire_ctx *ctx)
{
	return __wwlock(ww, ctx, 1, 1);
}

void
ww_mutex_unlock(struct ww_mutex *ww)
{
	struct ww_acquire_ctx *ctx;

	ctx = ww->ctx;
	if (ctx) {
		KASSERT(ctx->acquired > 0);
		--ctx->acquired;
		ww->ctx = NULL;
		ww->stamp = 0xFFFFFFFFFFFFFFFFLU;
	}
	rrw_exit(&ww->base);
	if (__sync_lock_test_and_set(&ww->blocked, 0))
		wakeup(ww);
}
