/* Public domain. */

#ifndef _LINUX_SEQLOCK_H
#define _LINUX_SEQLOCK_H

#include <sys/mutex.h>
#include <sys/atomic.h>

typedef struct {
	unsigned int seq;
	struct mutex lock;
} seqlock_t;

static inline void
seqlock_init(seqlock_t *sl)
{ 
	sl->seq = 0;
	mtx_init(&sl->lock, IPL_NONE);
}

static inline void
write_seqlock(seqlock_t *sl)
{
	mtx_enter(&sl->lock);
	sl->seq++;
	membar_producer();
}

static inline void
write_seqlock_irqsave(seqlock_t *sl, __unused long flags)
{
	mtx_enter(&sl->lock);
	sl->seq++;
	membar_producer();
}

static inline void
write_sequnlock(seqlock_t *sl)
{
	membar_producer();
	sl->seq++;
	mtx_leave(&sl->lock);
}

static inline void
write_sequnlock_irqrestore(seqlock_t *sl, __unused long flags)
{
	membar_producer();
	sl->seq++;
	mtx_leave(&sl->lock);
}

static inline unsigned int
read_seqbegin(seqlock_t *sl)
{
	return READ_ONCE(sl->seq);
}

static inline unsigned int
read_seqretry(seqlock_t *sl, unsigned int pos)
{
	return sl->seq != pos;
}

#endif
