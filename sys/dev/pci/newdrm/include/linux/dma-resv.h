/*
 * Header file for reservations for dma-buf and ttm
 *
 * Copyright(C) 2011 Linaro Limited. All rights reserved.
 * Copyright (C) 2012-2013 Canonical Ltd
 * Copyright (C) 2012 Texas Instruments
 *
 * Authors:
 * Rob Clark <robdclark@gmail.com>
 * Maarten Lankhorst <maarten.lankhorst@canonical.com>
 * Thomas Hellstrom <thellstrom-at-vmware-dot-com>
 *
 * Based on bo.c which bears the following copyright notice,
 * but is dual licensed:
 *
 * Copyright (c) 2006-2009 VMware, Inc., Palo Alto, CA., USA
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef _LINUX_RESERVATION_H
#define _LINUX_RESERVATION_H

#include <linux/ww_mutex.h>
#include <linux/dma-fence.h>
#include <linux/slab.h>
#include <linux/seqlock.h>
#include <linux/rcupdate.h>

extern struct ww_class reservation_ww_class;

/**
 * enum dma_resv_usage - how the fences from a dma_resv obj are used
 *
 * This enum describes the different use cases for a dma_resv object and
 * controls which fences are returned when queried.
 *
 * An important fact is that there is the order KERNEL<WRITE<READ<BOOKKEEP and
 * when the dma_resv object is asked for fences for one use case the fences
 * for the lower use case are returned as well.
 *
 * For example when asking for WRITE fences then the KERNEL fences are returned
 * as well. Similar when asked for READ fences then both WRITE and KERNEL
 * fences are returned as well.
 *
 * Already used fences can be promoted in the sense that a fence with
 * DMA_RESV_USAGE_BOOKKEEP could become DMA_RESV_USAGE_READ by adding it again
 * with this usage. But fences can never be degraded in the sense that a fence
 * with DMA_RESV_USAGE_WRITE could become DMA_RESV_USAGE_READ.
 */
enum dma_resv_usage {
	/**
	 * @DMA_RESV_USAGE_KERNEL: For in kernel memory management only.
	 *
	 * This should only be used for things like copying or clearing memory
	 * with a DMA hardware engine for the purpose of kernel memory
	 * management.
	 *
	 * Drivers *always* must wait for those fences before accessing the
	 * resource protected by the dma_resv object. The only exception for
	 * that is when the resource is known to be locked down in place by
	 * pinning it previously.
	 */
	DMA_RESV_USAGE_KERNEL,

	/**
	 * @DMA_RESV_USAGE_WRITE: Implicit write synchronization.
	 *
	 * This should only be used for userspace command submissions which add
	 * an implicit write dependency.
	 */
	DMA_RESV_USAGE_WRITE,

	/**
	 * @DMA_RESV_USAGE_READ: Implicit read synchronization.
	 *
	 * This should only be used for userspace command submissions which add
	 * an implicit read dependency.
	 */
	DMA_RESV_USAGE_READ,

	/**
	 * @DMA_RESV_USAGE_BOOKKEEP: No implicit sync.
	 *
	 * This should be used by submissions which don't want to participate in
	 * any implicit synchronization.
	 *
	 * The most common case are preemption fences, page table updates, TLB
	 * flushes as well as explicit synced user submissions.
	 *
	 * Explicit synced user user submissions can be promoted to
	 * DMA_RESV_USAGE_READ or DMA_RESV_USAGE_WRITE as needed using
	 * dma_buf_import_sync_file() when implicit synchronization should
	 * become necessary after initial adding of the fence.
	 */
	DMA_RESV_USAGE_BOOKKEEP
};

/**
 * dma_resv_usage_rw - helper for implicit sync
 * @write: true if we create a new implicit sync write
 *
 * This returns the implicit synchronization usage for write or read accesses,
 * see enum dma_resv_usage and &dma_buf.resv.
 */
static inline enum dma_resv_usage dma_resv_usage_rw(bool write)
{
	/* This looks confusing at first sight, but is indeed correct.
	 *
	 * The rational is that new write operations needs to wait for the
	 * existing read and write operations to finish.
	 * But a new read operation only needs to wait for the existing write
	 * operations to finish.
	 */
	return write ? DMA_RESV_USAGE_READ : DMA_RESV_USAGE_WRITE;
}

/**
 * struct dma_resv_list - a list of shared fences
 * @rcu: for internal use
 * @shared_count: table of shared fences
 * @shared_max: for growing shared fence table
 * @shared: shared fence table
 */
struct dma_resv_list {
	struct rcu_head rcu;
	u32 shared_count, shared_max;
	struct dma_fence __rcu *shared[];
};

/**
 * struct dma_resv - a reservation object manages fences for a buffer
 * @lock: update side lock
 * @seq: sequence count for managing RCU read-side synchronization
 * @fence_excl: the exclusive fence, if there is one currently
 * @fence: list of current shared fences
 */
struct dma_resv {
	struct ww_mutex lock;
	seqcount_t seq;

	struct dma_fence __rcu *fence_excl;
	struct dma_resv_list __rcu *fence;
};

#define dma_resv_held(obj) lockdep_is_held(&(obj)->lock.base)
#define dma_resv_assert_held(obj) lockdep_assert_held(&(obj)->lock.base)

#ifdef CONFIG_DEBUG_MUTEXES
void dma_resv_reset_shared_max(struct dma_resv *obj);
#else
static inline void dma_resv_reset_shared_max(struct dma_resv *obj) {}
#endif

/**
 * dma_resv_lock - lock the reservation object
 * @obj: the reservation object
 * @ctx: the locking context
 *
 * Locks the reservation object for exclusive access and modification. Note,
 * that the lock is only against other writers, readers will run concurrently
 * with a writer under RCU. The seqlock is used to notify readers if they
 * overlap with a writer.
 *
 * As the reservation object may be locked by multiple parties in an
 * undefined order, a #ww_acquire_ctx is passed to unwind if a cycle
 * is detected. See ww_mutex_lock() and ww_acquire_init(). A reservation
 * object may be locked by itself by passing NULL as @ctx.
 */
static inline int dma_resv_lock(struct dma_resv *obj,
				struct ww_acquire_ctx *ctx)
{
	return ww_mutex_lock(&obj->lock, ctx);
}

/**
 * dma_resv_lock_interruptible - lock the reservation object
 * @obj: the reservation object
 * @ctx: the locking context
 *
 * Locks the reservation object interruptible for exclusive access and
 * modification. Note, that the lock is only against other writers, readers
 * will run concurrently with a writer under RCU. The seqlock is used to
 * notify readers if they overlap with a writer.
 *
 * As the reservation object may be locked by multiple parties in an
 * undefined order, a #ww_acquire_ctx is passed to unwind if a cycle
 * is detected. See ww_mutex_lock() and ww_acquire_init(). A reservation
 * object may be locked by itself by passing NULL as @ctx.
 */
static inline int dma_resv_lock_interruptible(struct dma_resv *obj,
					      struct ww_acquire_ctx *ctx)
{
	return ww_mutex_lock_interruptible(&obj->lock, ctx);
}

/**
 * dma_resv_lock_slow - slowpath lock the reservation object
 * @obj: the reservation object
 * @ctx: the locking context
 *
 * Acquires the reservation object after a die case. This function
 * will sleep until the lock becomes available. See dma_resv_lock() as
 * well.
 */
static inline void dma_resv_lock_slow(struct dma_resv *obj,
				      struct ww_acquire_ctx *ctx)
{
	ww_mutex_lock_slow(&obj->lock, ctx);
}

/**
 * dma_resv_lock_slow_interruptible - slowpath lock the reservation
 * object, interruptible
 * @obj: the reservation object
 * @ctx: the locking context
 *
 * Acquires the reservation object interruptible after a die case. This function
 * will sleep until the lock becomes available. See
 * dma_resv_lock_interruptible() as well.
 */
static inline int dma_resv_lock_slow_interruptible(struct dma_resv *obj,
						   struct ww_acquire_ctx *ctx)
{
	return ww_mutex_lock_slow_interruptible(&obj->lock, ctx);
}

/**
 * dma_resv_trylock - trylock the reservation object
 * @obj: the reservation object
 *
 * Tries to lock the reservation object for exclusive access and modification.
 * Note, that the lock is only against other writers, readers will run
 * concurrently with a writer under RCU. The seqlock is used to notify readers
 * if they overlap with a writer.
 *
 * Also note that since no context is provided, no deadlock protection is
 * possible.
 *
 * Returns true if the lock was acquired, false otherwise.
 */
static inline bool __must_check dma_resv_trylock(struct dma_resv *obj)
{
	return ww_mutex_trylock(&obj->lock, NULL);
}

/**
 * dma_resv_is_locked - is the reservation object locked
 * @obj: the reservation object
 *
 * Returns true if the mutex is locked, false if unlocked.
 */
static inline bool dma_resv_is_locked(struct dma_resv *obj)
{
	return ww_mutex_is_locked(&obj->lock);
}

/**
 * dma_resv_locking_ctx - returns the context used to lock the object
 * @obj: the reservation object
 *
 * Returns the context used to lock a reservation object or NULL if no context
 * was used or the object is not locked at all.
 */
static inline struct ww_acquire_ctx *dma_resv_locking_ctx(struct dma_resv *obj)
{
	return READ_ONCE(obj->lock.ctx);
}

/**
 * dma_resv_unlock - unlock the reservation object
 * @obj: the reservation object
 *
 * Unlocks the reservation object following exclusive access.
 */
static inline void dma_resv_unlock(struct dma_resv *obj)
{
	dma_resv_reset_shared_max(obj);
	ww_mutex_unlock(&obj->lock);
}

/**
 * dma_resv_excl_fence - return the object's exclusive fence
 * @obj: the reservation object
 *
 * Returns the exclusive fence (if any). Caller must either hold the objects
 * through dma_resv_lock() or the RCU read side lock through rcu_read_lock(),
 * or one of the variants of each
 *
 * RETURNS
 * The exclusive fence or NULL
 */
static inline struct dma_fence *
dma_resv_excl_fence(struct dma_resv *obj)
{
	return rcu_dereference_check(obj->fence_excl, dma_resv_held(obj));
}

/**
 * dma_resv_get_excl_unlocked - get the reservation object's
 * exclusive fence, without lock held.
 * @obj: the reservation object
 *
 * If there is an exclusive fence, this atomically increments it's
 * reference count and returns it.
 *
 * RETURNS
 * The exclusive fence or NULL if none
 */
static inline struct dma_fence *
dma_resv_get_excl_unlocked(struct dma_resv *obj)
{
	struct dma_fence *fence;

	if (!rcu_access_pointer(obj->fence_excl))
		return NULL;

	rcu_read_lock();
	fence = dma_fence_get_rcu_safe(&obj->fence_excl);
	rcu_read_unlock();

	return fence;
}

/**
 * dma_resv_shared_list - get the reservation object's shared fence list
 * @obj: the reservation object
 *
 * Returns the shared fence list. Caller must either hold the objects
 * through dma_resv_lock() or the RCU read side lock through rcu_read_lock(),
 * or one of the variants of each
 */
static inline struct dma_resv_list *dma_resv_shared_list(struct dma_resv *obj)
{
	return rcu_dereference_check(obj->fence, dma_resv_held(obj));
}

void dma_resv_init(struct dma_resv *obj);
void dma_resv_fini(struct dma_resv *obj);
int dma_resv_reserve_fences(struct dma_resv *obj, unsigned int num_fences);
void dma_resv_add_shared_fence(struct dma_resv *obj, struct dma_fence *fence);
void dma_resv_add_excl_fence(struct dma_resv *obj, struct dma_fence *fence);
int dma_resv_get_fences(struct dma_resv *obj, struct dma_fence **pfence_excl,
			unsigned *pshared_count, struct dma_fence ***pshared);
int dma_resv_copy_fences(struct dma_resv *dst, struct dma_resv *src);
long dma_resv_wait_timeout(struct dma_resv *obj, bool wait_all, bool intr,
			   unsigned long timeout);
bool dma_resv_test_signaled(struct dma_resv *obj, bool test_all);

#endif /* _LINUX_RESERVATION_H */
