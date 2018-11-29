/* Public domain. */

#ifndef _LINUX_SPINLOCK_H
#define _LINUX_SPINLOCK_H

#include <linux/spinlock_types.h>

static inline void
_spin_lock_irqsave(struct mutex *mtxp, __unused unsigned long flags
    LOCK_FL_VARS)
{
	_mtx_enter(mtxp LOCK_FL_ARGS);
}
static inline void
_spin_lock_irqsave_nested(struct mutex *mtxp, __unused unsigned long flags,
    __unused int subclass LOCK_FL_VARS)
{
	_mtx_enter(mtxp LOCK_FL_ARGS);
}
static inline void
_spin_unlock_irqrestore(struct mutex *mtxp, __unused unsigned long flags
    LOCK_FL_VARS)
{
	_mtx_leave(mtxp LOCK_FL_ARGS);
}
#define spin_lock_irqsave(m, fl)	\
	_spin_lock_irqsave(m, fl LOCK_FILE_LINE)
#define spin_lock_irqsave_nested(m, fl, subc)	\
	_spin_lock_irqsave_nested(m, fl, subc LOCK_FILE_LINE)
#define spin_unlock_irqrestore(m, fl)	\
	_spin_unlock_irqrestore(m, fl LOCK_FILE_LINE)

#define spin_lock(mtxp)			mtx_enter(mtxp)
#define spin_lock_nested(mtxp, l)	mtx_enter(mtxp)
#define spin_unlock(mtxp)		mtx_leave(mtxp)
#define spin_lock_irq(mtxp)		mtx_enter(mtxp)
#define spin_unlock_irq(mtxp)		mtx_leave(mtxp)
#define assert_spin_locked(mtxp)	MUTEX_ASSERT_LOCKED(mtxp)

#endif
