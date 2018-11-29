/*	$OpenBSD: drm_linux.h,v 1.92 2018/10/31 08:50:25 kettenis Exp $	*/
/*
 * Copyright (c) 2013, 2014, 2015 Mark Kettenis
 * Copyright (c) 2017 Martin Pieuchot
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

#ifndef _DRM_LINUX_H_
#define _DRM_LINUX_H_

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/signalvar.h>
#include <sys/stdint.h>
#include <sys/systm.h>
#include <sys/task.h>
#include <sys/time.h>
#include <sys/timeout.h>
#include <sys/tree.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

#include <ddb/db_var.h>

#include <dev/i2c/i2cvar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/atomic.h>
#include <linux/list.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/seq_file.h>
#include <linux/wait.h>
#include <linux/dma-fence.h>
#include <linux/dma-fence-array.h>
#include <linux/compiler.h>
#include <linux/bug.h>
#include <linux/completion.h>
#include <linux/kobject.h>
#include <linux/lockdep.h>
#include <linux/bitmap.h>
#include <linux/idr.h>
#include <linux/hashtable.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/llist.h>
#include <linux/irq.h>
#include <video/mipi_display.h>

/* The Linux code doesn't meet our usual standards! */
#ifdef __clang__
#pragma clang diagnostic ignored "-Wenum-conversion"
#pragma clang diagnostic ignored "-Winitializer-overrides"
#pragma clang diagnostic ignored "-Wtautological-compare"
#pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
#pragma clang diagnostic ignored "-Wunused-const-variable"
#pragma clang diagnostic ignored "-Wincompatible-pointer-types-discards-qualifiers"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-variable"
#else
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#endif

#define STUB() do { printf("%s: stub\n", __func__); } while(0)

typedef uint16_t __le16;
typedef uint16_t __be16;
typedef uint32_t __le32;
typedef uint32_t __be32;

#define CONFIG_DRM_FBDEV_OVERALLOC	0
#define CONFIG_DRM_I915_DEBUG		0
#define CONFIG_DRM_I915_DEBUG_GEM	0
#define CONFIG_PM			0

#define __printf(x, y)

#define barrier()		__asm __volatile("" : : : "memory")

#define ACCESS_ONCE(x)		(*(volatile __typeof(x) *)&(x))

#define READ_ONCE(x) ({		\
	__typeof(x) __tmp = ({	\
		barrier();	\
		ACCESS_ONCE(x);	\
	});			\
	barrier();		\
	__tmp;			\
})

#define WRITE_ONCE(x, v) do {	\
	barrier();		\
	ACCESS_ONCE(x) = (v);	\
	barrier();		\
} while(0)

#define uninitialized_var(x) x

#if BYTE_ORDER == BIG_ENDIAN
#define __BIG_ENDIAN
#else
#define __LITTLE_ENDIAN
#endif

#define le16_to_cpu(x) letoh16(x)
#define le32_to_cpu(x) letoh32(x)
#define be16_to_cpu(x) betoh16(x)
#define be32_to_cpu(x) betoh32(x)
#define le16_to_cpup(x)	lemtoh16(x)
#define le32_to_cpup(x)	lemtoh32(x)
#define be16_to_cpup(x)	bemtoh16(x)
#define be32_to_cpup(x)	bemtoh32(x)
#define get_unaligned_le32(x)	lemtoh32(x)
#define cpu_to_le16(x) htole16(x)
#define cpu_to_le32(x) htole32(x)
#define cpu_to_be16(x) htobe16(x)
#define cpu_to_be32(x) htobe32(x)

#define lower_32_bits(n)	((u32)(n))
#define upper_32_bits(_val)	((u32)(((_val) >> 16) >> 16))
#define DMA_BIT_MASK(n) (((n) == 64) ? ~0ULL : (1ULL<<(n)) -1)
#define BITS_TO_LONGS(x)	howmany((x), 8 * sizeof(long))
#ifdef __LP64__
#define BITS_PER_LONG		64
#else
#define BITS_PER_LONG		32
#endif
#define BITS_PER_LONG_LONG	64
#define GENMASK(h, l)		(((~0UL) >> (BITS_PER_LONG - (h) - 1)) & ((~0UL) << (l)))
#define GENMASK_ULL(h, l)	(((~0ULL) >> (BITS_PER_LONG_LONG - (h) - 1)) & ((~0ULL) << (l)))

#define DECLARE_BITMAP(x, y)	unsigned long x[BITS_TO_LONGS(y)];

static inline uint64_t
sign_extend64(uint64_t value, int index)
{
	uint8_t shift = 63 - index;
	return ((int64_t)(value << shift) >> shift);
}

/* 2^32 * ((sqrt(5) - 1) / 2) from Knuth */
#define GOLDEN_RATIO_32	0x9e3779b9

static inline uint32_t
hash_32(uint32_t val, unsigned int bits)
{
	return (val * GOLDEN_RATIO_32) >> (32 - bits);
}

#define IS_ENABLED(x) x - 0

#define IS_BUILTIN(x) 1

struct device_node;

struct device_driver {
	struct device *dev;
};

#define dev_get_drvdata(x)	NULL
#define dev_set_drvdata(x, y)
#define dev_name(dev)		""

#define devm_kzalloc(x, y, z)	kzalloc(y, z)

#define ARRAY_SIZE nitems

#define KERN_INFO	""
#define KERN_WARNING	""
#define KERN_NOTICE	""
#define KERN_DEBUG	""
#define KERN_CRIT	""
#define KERN_ERR	""

#define KBUILD_MODNAME "drm"

#define UTS_RELEASE	""

#define TASK_COMM_LEN	(MAXCOMLEN + 1)

struct va_format {
	const char *fmt;
	va_list *va;
};

#ifndef pr_fmt
#define pr_fmt(fmt) fmt
#endif

#define printk_once(fmt, arg...) ({		\
	static int __warned;			\
	if (!__warned) {			\
		printf(fmt, ## arg);		\
		__warned = 1;			\
	}					\
})

#define printk(fmt, arg...)	printf(fmt, ## arg)
#define pr_warn(fmt, arg...)	printf(pr_fmt(fmt), ## arg)
#define pr_warn_once(fmt, arg...)	printk_once(pr_fmt(fmt), ## arg)
#define pr_notice(fmt, arg...)	printf(pr_fmt(fmt), ## arg)
#define pr_crit(fmt, arg...)	printf(pr_fmt(fmt), ## arg)
#define pr_err(fmt, arg...)	printf(pr_fmt(fmt), ## arg)
#define pr_cont(fmt, arg...)	printf(pr_fmt(fmt), ## arg)

#ifdef DRMDEBUG
#define pr_info(fmt, arg...)	printf(pr_fmt(fmt), ## arg)
#define pr_info_once(fmt, arg...)	printk_once(pr_fmt(fmt), ## arg)
#define pr_debug(fmt, arg...)	printf(pr_fmt(fmt), ## arg)
#else
#define pr_info(fmt, arg...)	do { } while(0)
#define pr_info_once(fmt, arg...)	do { } while(0)
#define pr_debug(fmt, arg...)	do { } while(0)
#endif

#define dev_warn(dev, fmt, arg...)				\
	printf("drm:pid%d:%s *WARNING* " fmt, curproc->p_p->ps_pid,	\
	    __func__ , ## arg)
#define dev_notice(dev, fmt, arg...)				\
	printf("drm:pid%d:%s *NOTICE* " fmt, curproc->p_p->ps_pid,	\
	    __func__ , ## arg)
#define dev_crit(dev, fmt, arg...)				\
	printf("drm:pid%d:%s *ERROR* " fmt, curproc->p_p->ps_pid,	\
	    __func__ , ## arg)
#define dev_err(dev, fmt, arg...)				\
	printf("drm:pid%d:%s *ERROR* " fmt, curproc->p_p->ps_pid,	\
	    __func__ , ## arg)
#define dev_printk(level, dev, fmt, arg...)				\
	printf("drm:pid%d:%s *PRINTK* " fmt, curproc->p_p->ps_pid,	\
	    __func__ , ## arg)

#ifdef DRMDEBUG
#define dev_info(dev, fmt, arg...)				\
	printf("drm: " fmt, ## arg)
#define dev_debug(dev, fmt, arg...)				\
	printf("drm:pid%d:%s *DEBUG* " fmt, curproc->p_p->ps_pid,	\
	    __func__ , ## arg)
#else
#define dev_info(dev, fmt, arg...) 				\
	    do { } while(0)
#define dev_debug(dev, fmt, arg...) 				\
	    do { } while(0)
#endif

#define DEFINE_RATELIMIT_STATE(name, interval, burst) \
	int name __used = 1;

#define __ratelimit(x)	(1)

enum {
	DUMP_PREFIX_NONE,
	DUMP_PREFIX_ADDRESS,
	DUMP_PREFIX_OFFSET
};

void print_hex_dump(const char *, const char *, int, int, int,
	 const void *, size_t, bool);

#define scnprintf(str, size, fmt, arg...) snprintf(str, size, fmt, ## arg)

#define TP_PROTO(x...) x

#define DEFINE_EVENT(template, name, proto, args) \
static inline void trace_##name(proto) {}

#define DEFINE_EVENT_PRINT(template, name, proto, args, print) \
static inline void trace_##name(proto) {}

#define TRACE_EVENT(name, proto, args, tstruct, assign, print) \
static inline void trace_##name(proto) {}

#define TRACE_EVENT_CONDITION(name, proto, args, cond, tstruct, assign, print) \
static inline void trace_##name(proto) {}

#define DECLARE_EVENT_CLASS(name, proto, args, tstruct, assign, print) \
static inline void trace_##name(proto) {}

#define IS_ERR_VALUE(x) unlikely((x) >= (unsigned long)-ELAST)

static inline void *
ERR_PTR(long error)
{
	return (void *) error;
}

static inline long
PTR_ERR(const void *ptr)
{
	return (long) ptr;
}

static inline long
IS_ERR(const void *ptr)
{
        return IS_ERR_VALUE((unsigned long)ptr);
}

static inline long
IS_ERR_OR_NULL(const void *ptr)
{
        return !ptr || IS_ERR_VALUE((unsigned long)ptr);
}

static inline void *
ERR_CAST(const void *ptr)
{
	return (void *)ptr;
}

static inline int
PTR_ERR_OR_ZERO(const void *ptr)
{
	return IS_ERR(ptr)? PTR_ERR(ptr) : 0;
}

#ifndef __DECONST
#define __DECONST(type, var)    ((type)(__uintptr_t)(const void *)(var))
#endif

typedef struct rwlock rwlock_t;
typedef struct mutex spinlock_t;
#define DEFINE_SPINLOCK(x)	struct mutex x
#define DEFINE_MUTEX(x)		struct rwlock x

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
#define mutex_lock_interruptible(rwl)	-rw_enter(rwl, RW_WRITE | RW_INTR)
#define mutex_lock(rwl)			rw_enter_write(rwl)
#define mutex_lock_nest_lock(rwl, sub)	rw_enter_write(rwl)
#define mutex_lock_nested(rwl, sub)	rw_enter_write(rwl)
#define mutex_trylock(rwl)		(rw_enter(rwl, RW_WRITE | RW_NOSLEEP) == 0)
#define mutex_unlock(rwl)		rw_exit_write(rwl)
#define mutex_is_locked(rwl)		(rw_status(rwl) == RW_WRITE)
#define mutex_destroy(rwl)
#define down_read(rwl)			rw_enter_read(rwl)
#define down_read_trylock(rwl)		(rw_enter(rwl, RW_READ | RW_NOSLEEP) == 0)
#define up_read(rwl)			rw_exit_read(rwl)
#define down_write(rwl)			rw_enter_write(rwl)
#define up_write(rwl)			rw_exit_write(rwl)
#define read_lock(rwl)			rw_enter_read(rwl)
#define read_unlock(rwl)		rw_exit_read(rwl)
#define write_lock(rwl)			rw_enter_write(rwl)
#define write_unlock(rwl)		rw_exit_write(rwl)

#define IRQF_SHARED	0

#define local_irq_save(x)		(x) = splhigh()
#define local_irq_restore(x)		splx((x))

#define request_irq(irq, hdlr, flags, name, dev)	(0)
#define free_irq(irq, dev)
#define synchronize_irq(x)

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

#define local_bh_disable()
#define local_bh_enable()

typedef uint64_t async_cookie_t;
typedef void (*async_func_t) (void *, async_cookie_t);

static inline async_cookie_t
async_schedule(async_func_t func, void *data)
{
	func(data, 0);
	return 0;
}

#define local_irq_disable()	intr_disable()
#define local_irq_enable()	intr_enable()
#define disable_irq(x)		intr_disable()
#define enable_irq(x)		intr_enable()

#define setup_timer(x, y, z)	timeout_set((x), (void (*)(void *))(y), (void *)(z))
#define mod_timer(x, y)		timeout_add((x), (y - jiffies))
#define mod_timer_pinned(x, y)	timeout_add((x), (y - jiffies))
#define del_timer_sync(x)	timeout_del((x))
#define timer_pending(x)	timeout_pending((x))

#define signal_pending_state(x, y) CURSIG(curproc)
#define signal_pending(y) CURSIG(curproc)

#define NSEC_PER_USEC	1000L
#define NSEC_PER_MSEC	1000000L
#define NSEC_PER_SEC	1000000000L
#define KHZ2PICOS(a)	(1000000000UL/(a))

extern struct timespec ns_to_timespec(const int64_t);
extern int64_t timeval_to_ms(const struct timeval *);
extern int64_t timeval_to_ns(const struct timeval *);
extern int64_t timeval_to_us(const struct timeval *);
extern struct timeval ns_to_timeval(const int64_t);

struct timespec64 {
	time_t	tv_sec;
	long	tv_nsec;
};

static inline struct timespec
timespec_sub(struct timespec t1, struct timespec t2)
{
	struct timespec diff;

	timespecsub(&t1, &t2, &diff);
	return diff;
}

#define time_in_range(x, min, max) ((x) >= (min) && (x) <= (max))

extern volatile unsigned long jiffies;
#define jiffies_64 jiffies /* XXX */
#undef HZ
#define HZ	hz

#define MAX_JIFFY_OFFSET	((INT_MAX >> 1) - 1)

static inline unsigned long
round_jiffies_up(unsigned long j)
{
	return roundup(j, hz);
}

static inline unsigned long
round_jiffies_up_relative(unsigned long j)
{
	return roundup(j, hz);
}

#define jiffies_to_msecs(x)	(((uint64_t)(x)) * 1000 / hz)
#define jiffies_to_usecs(x)	(((uint64_t)(x)) * 1000000 / hz)
#define msecs_to_jiffies(x)	(((uint64_t)(x)) * hz / 1000)
#define usecs_to_jiffies(x)	(((uint64_t)(x)) * hz / 1000000)
#define nsecs_to_jiffies(x)	(((uint64_t)(x)) * hz / 1000000000)
#define nsecs_to_jiffies64(x)	(((uint64_t)(x)) * hz / 1000000000)
#define get_jiffies_64()	jiffies
#define time_after(a,b)		((long)(b) - (long)(a) < 0)
#define time_after32(a,b)	((uint32_t)(b) - (uint32_t)(a) < 0)
#define time_after_eq(a,b)	((long)(b) - (long)(a) <= 0)
#define time_before(a,b)	((long)(a) - (long)(b) < 0)
#define get_seconds()		time_second
#define getrawmonotonic(x)	nanouptime(x)

static inline void
set_normalized_timespec(struct timespec *ts, time_t sec, int64_t nsec)
{
	while (nsec > NSEC_PER_SEC) {
		nsec -= NSEC_PER_SEC;
		sec++;
	}

	ts->tv_sec = sec;
	ts->tv_nsec = nsec;
}

static inline int64_t
timespec_to_ns(const struct timespec *ts)
{
	return ((ts->tv_sec * NSEC_PER_SEC) + ts->tv_nsec);
}

static inline unsigned long
timespec_to_jiffies(const struct timespec *ts)
{
	long long to_ticks;

	to_ticks = (long long)hz * ts->tv_sec + ts->tv_nsec / (tick * 1000);
	if (to_ticks > INT_MAX)
		to_ticks = INT_MAX;

	return ((int)to_ticks);
}

static inline int
timespec_valid(const struct timespec *ts)
{
	if (ts->tv_sec < 0 || ts->tv_sec > 100000000 ||
	    ts->tv_nsec < 0 || ts->tv_nsec >= 1000000000)
		return (0);
	return (1);
}

typedef struct timeval ktime_t;

static inline struct timeval
ktime_get(void)
{
	struct timeval tv;
	
	getmicrouptime(&tv);
	return tv;
}

static inline struct timeval
ktime_get_raw(void)
{
	struct timeval tv;
	
	microuptime(&tv);
	return tv;
}

static inline struct timeval
ktime_get_monotonic_offset(void)
{
	struct timeval tv = {0, 0};
	return tv;
}

static inline int64_t
ktime_to_ms(struct timeval tv)
{
	return timeval_to_ms(&tv);
}

static inline int64_t
ktime_to_us(struct timeval tv)
{
	return timeval_to_us(&tv);
}

static inline int64_t
ktime_to_ns(struct timeval tv)
{
	return timeval_to_ns(&tv);
}

static inline int64_t
ktime_get_raw_ns(void)
{
	return ktime_to_ns(ktime_get());
}

#define ktime_to_timeval(tv) (tv)

static inline struct timespec64
ktime_to_timespec64(struct timeval tv)
{
	struct timespec64 ts;
	ts.tv_sec = tv.tv_sec;
	ts.tv_nsec = tv.tv_usec * NSEC_PER_USEC;
	return ts;
}

static inline struct timeval
ktime_sub(struct timeval a, struct timeval b)
{
	struct timeval res;
	timersub(&a, &b, &res);
	return res;
}

static inline struct timeval
ktime_add(struct timeval a, struct timeval b)
{
	struct timeval res;
	timeradd(&a, &b, &res);
	return res;
}

static inline struct timeval
ktime_add_ns(struct timeval tv, int64_t ns)
{
	return ns_to_timeval(timeval_to_ns(&tv) + ns);
}

static inline struct timeval
ktime_sub_ns(struct timeval tv, int64_t ns)
{
	return ns_to_timeval(timeval_to_ns(&tv) - ns);
}

static inline int64_t
ktime_us_delta(struct timeval a, struct timeval b)
{
	return ktime_to_us(ktime_sub(a, b));
}

static inline int64_t
ktime_ms_delta(struct timeval a, struct timeval b)
{
	return ktime_to_ms(ktime_sub(a, b));
}

static inline bool
ktime_after(const struct timeval a, const struct timeval b)
{
	return timercmp(&a, &b, >);
}

#define ktime_mono_to_real(x) (x)
#define ktime_get_real() ktime_get()
#define ktime_get_boottime() ktime_get()

#define do_gettimeofday(tv) getmicrouptime(tv)

static inline int64_t
ktime_get_real_seconds(void)
{
	return ktime_get().tv_sec;
}

static inline struct timeval
ns_to_ktime(uint64_t ns)
{
	return ns_to_timeval(ns);
}

static inline uint64_t
local_clock(void)
{
	struct timespec ts;
	nanouptime(&ts);
	return (ts.tv_sec * NSEC_PER_SEC) + ts.tv_nsec;
}

#define PageHighMem(x)	0

#define array_size(x, y) ((x) * (y))

static inline void *
kvmalloc_array(size_t n, size_t size, int flags)
{
	if (n == 0 || SIZE_MAX / n < size)
		return NULL;
	return malloc(n * size, M_DRM, flags);
}

static inline void *
kvzalloc(size_t size, int flags)
{
	return malloc(size, M_DRM, flags | M_ZERO);
}

static inline void
kvfree(const void *objp)
{
	free((void *)objp, M_DRM, 0);
}

static inline void *
kmemdup(const void *src, size_t len, int flags)
{
	void *p = malloc(len, M_DRM, flags);
	if (p)
		memcpy(p, src, len);
	return (p);
}

static inline void *
kstrdup(const char *str, int flags)
{
	size_t len;
	char *p;

	len = strlen(str) + 1;
	p = malloc(len, M_DRM, flags);
	if (p)
		memcpy(p, str, len);
	return (p);
}

static inline char *
kasprintf(int flags, const char *fmt, ...)
{
	char *buf;
	size_t len;
	va_list ap;

	va_start(ap, fmt);
	len = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);

	buf = kmalloc(len, flags);
	if (buf) {
		va_start(ap, fmt);
		vsnprintf(buf, len, fmt, ap);
		va_end(ap);
	}

	return buf;
}

static inline char *
kvasprintf(int flags, const char *fmt, va_list ap)
{
	char *buf;
	size_t len;

	len = vsnprintf(NULL, 0, fmt, ap);

	buf = kmalloc(len, flags);
	if (buf) {
		vsnprintf(buf, len, fmt, ap);
	}

	return buf;
}

static inline void *
vmalloc(unsigned long size)
{
	return malloc(size, M_DRM, M_WAITOK | M_CANFAIL);
}

static inline void *
vzalloc(unsigned long size)
{
	return malloc(size, M_DRM, M_WAITOK | M_CANFAIL | M_ZERO);
}

static inline void
vfree(void *objp)
{
	free(objp, M_DRM, 0);
}

static inline bool
refcount_dec_and_test(uint32_t *p)
{
	return atomic_dec_and_test(p);
}

#define preempt_enable()
#define preempt_disable()

struct sync_file {
};

static inline struct dma_fence *
sync_file_get_fence(int fd)
{
	STUB();
	return NULL;
}

static inline struct sync_file *
sync_file_create(struct dma_fence *fence)
{
	STUB();
	return NULL;
}

struct notifier_block {
	void *notifier_call;
};

#define register_reboot_notifier(x)
#define unregister_reboot_notifier(x)

#define ATOMIC_INIT_NOTIFIER_HEAD(x)

#define SYS_RESTART 0

#define min_t(t, a, b) ({ \
	t __min_a = (a); \
	t __min_b = (b); \
	__min_a < __min_b ? __min_a : __min_b; })

#define max_t(t, a, b) ({ \
	t __max_a = (a); \
	t __max_b = (b); \
	__max_a > __max_b ? __max_a : __max_b; })

#define clamp_t(t, x, a, b) min_t(t, max_t(t, x, a), b)
#define clamp(x, a, b) clamp_t(__typeof(x), x, a, b)
#define clamp_val(x, a, b) clamp_t(__typeof(x), x, a, b)

#define min(a, b) MIN(a, b)
#define max(a, b) MAX(a, b)
#define min3(x, y, z) MIN(x, MIN(y, z))
#define max3(x, y, z) MAX(x, MAX(y, z))

#define do_div(n, base) ({				\
	uint32_t __base = (base);			\
	uint32_t __rem = ((uint64_t)(n)) % __base;	\
	(n) = ((uint64_t)(n)) / __base;			\
	__rem;						\
})

static inline uint64_t
div_u64(uint64_t x, uint32_t y)
{
	return (x / y);
}

static inline int64_t
div_s64(int64_t x, int64_t y)
{
	return (x / y);
}

static inline uint64_t
div64_u64(uint64_t x, uint64_t y)
{
	return (x / y);
}

static inline uint64_t
div64_u64_rem(uint64_t x, uint64_t y, uint64_t *rem)
{
	*rem = x % y;
	return (x / y);
}

static inline int64_t
div64_s64(int64_t x, int64_t y)
{
	return (x / y);
}

static inline uint64_t
mul_u32_u32(uint32_t x, uint32_t y)
{
	return (uint64_t)x * y;
}

static inline uint64_t
mul_u64_u32_div(uint64_t x, uint32_t y, uint32_t div)
{
	return (x * y) / div;
}

#define mult_frac(x, n, d) (((x) * (n)) / (d))
#define order_base_2(x) drm_order(x)

static inline int64_t
abs64(int64_t x)
{
	return (x < 0 ? -x : x);
}

#define user_access_begin()
#define user_access_end()

static inline unsigned long
__copy_to_user(void *to, const void *from, unsigned len)
{
	if (copyout(from, to, len))
		return len;
	return 0;
}

static inline unsigned long
copy_to_user(void *to, const void *from, unsigned len)
{
	return __copy_to_user(to, from, len);
}

static inline unsigned long
__copy_from_user(void *to, const void *from, unsigned len)
{
	if (copyin(from, to, len))
		return len;
	return 0;
}

static inline unsigned long
copy_from_user(void *to, const void *from, unsigned len)
{
	return __copy_from_user(to, from, len);
}

#define get_user(x, ptr)	-copyin(ptr, &(x), sizeof(x))
#define put_user(x, ptr) ({				\
	__typeof((x)) __tmp = (x);			\
	-copyout(&(__tmp), ptr, sizeof(__tmp));		\
})
#define __get_user(x, ptr)	get_user((x), (ptr))
#define __put_user(x, ptr)	put_user((x), (ptr))

#define unsafe_put_user(x, ptr, err) ({				\
	__typeof((x)) __tmp = (x);				\
	if (copyout(&(__tmp), ptr, sizeof(__tmp)) != 0)		\
		goto err;					\
})

#define u64_to_user_ptr(x)	((void *)(uintptr_t)(x))

#define console_lock()
#define console_trylock()	1
#define console_unlock()

#ifndef PCI_MEM_START
#define PCI_MEM_START	0
#endif

#ifndef PCI_MEM_END
#define PCI_MEM_END	0xffffffff
#endif

enum dmi_field {
        DMI_NONE,
        DMI_BIOS_VENDOR,
        DMI_BIOS_VERSION,
        DMI_BIOS_DATE,
        DMI_SYS_VENDOR,
        DMI_PRODUCT_NAME,
        DMI_PRODUCT_VERSION,
        DMI_PRODUCT_SERIAL,
        DMI_PRODUCT_UUID,
        DMI_BOARD_VENDOR,
        DMI_BOARD_NAME,
        DMI_BOARD_VERSION,
        DMI_BOARD_SERIAL,
        DMI_BOARD_ASSET_TAG,
        DMI_CHASSIS_VENDOR,
        DMI_CHASSIS_TYPE,
        DMI_CHASSIS_VERSION,
        DMI_CHASSIS_SERIAL,
        DMI_CHASSIS_ASSET_TAG,
        DMI_STRING_MAX,
};

struct dmi_strmatch {
	unsigned char slot;
	char substr[79];
};

struct dmi_system_id {
        int (*callback)(const struct dmi_system_id *);
        const char *ident;
        struct dmi_strmatch matches[4];
};
#define	DMI_MATCH(a, b) {(a), (b)}
#define	DMI_EXACT_MATCH(a, b) {(a), (b)}
int dmi_check_system(const struct dmi_system_id *);
bool dmi_match(int, const char *);

struct resource {
	u_long	start;
	u_long	end;
};

static inline resource_size_t
resource_size(const struct resource *r)
{
	return r->end - r->start + 1;
}

#define DEFINE_RES_MEM(_start, _size)		\
	{					\
		.start = (_start),		\
		.end = (_start) + (_size) - 1,	\
	}

struct pci_dev;

struct pci_bus {
	pci_chipset_tag_t pc;
	unsigned char	number;
	pcitag_t	*bridgetag;
	struct pci_dev	*self;
};

struct pci_dev {
	struct pci_bus	_bus;
	struct pci_bus	*bus;

	unsigned int	devfn;
	uint16_t	vendor;
	uint16_t	device;
	uint16_t	subsystem_vendor;
	uint16_t	subsystem_device;
	uint8_t		revision;

	pci_chipset_tag_t pc;
	pcitag_t	tag;
	struct pci_softc *pci;

	int		irq;
	int		msi_enabled;
	uint8_t		no_64bit_msi;
};
#define PCI_ANY_ID (uint16_t) (~0U)

#define PCI_VENDOR_ID_APPLE	PCI_VENDOR_APPLE
#define PCI_VENDOR_ID_ASUSTEK	PCI_VENDOR_ASUSTEK
#define PCI_VENDOR_ID_ATI	PCI_VENDOR_ATI
#define PCI_VENDOR_ID_DELL	PCI_VENDOR_DELL
#define PCI_VENDOR_ID_HP	PCI_VENDOR_HP
#define PCI_VENDOR_ID_IBM	PCI_VENDOR_IBM
#define PCI_VENDOR_ID_INTEL	PCI_VENDOR_INTEL
#define PCI_VENDOR_ID_SONY	PCI_VENDOR_SONY
#define PCI_VENDOR_ID_VIA	PCI_VENDOR_VIATECH

#define PCI_DEVICE_ID_ATI_RADEON_QY	PCI_PRODUCT_ATI_RADEON_QY

#define PCI_SUBVENDOR_ID_REDHAT_QUMRANET	0x1af4
#define PCI_SUBDEVICE_ID_QEMU			0x1100

#define PCI_DEVFN(slot, func)	((slot) << 3 | (func))
#define PCI_SLOT(devfn)		((devfn) >> 3)
#define PCI_FUNC(devfn)		((devfn) & 0x7)

#define pci_dev_put(x)

#define PCI_EXP_DEVSTA		0x0a
#define PCI_EXP_DEVSTA_TRPND	0x0020
#define PCI_EXP_LNKCAP		0x0c
#define PCI_EXP_LNKCAP_CLKPM	0x00040000
#define PCI_EXP_LNKCTL		0x10
#define PCI_EXP_LNKCTL_HAWD	0x0200
#define PCI_EXP_LNKCTL2		0x30

static inline int
pci_read_config_dword(struct pci_dev *pdev, int reg, u32 *val)
{
	*val = pci_conf_read(pdev->pc, pdev->tag, reg);
	return 0;
} 

static inline int
pci_read_config_word(struct pci_dev *pdev, int reg, u16 *val)
{
	uint32_t v;

	v = pci_conf_read(pdev->pc, pdev->tag, (reg & ~0x2));
	*val = (v >> ((reg & 0x2) * 8));
	return 0;
} 

static inline int
pci_read_config_byte(struct pci_dev *pdev, int reg, u8 *val)
{
	uint32_t v;

	v = pci_conf_read(pdev->pc, pdev->tag, (reg & ~0x3));
	*val = (v >> ((reg & 0x3) * 8));
	return 0;
} 

static inline int
pci_write_config_dword(struct pci_dev *pdev, int reg, u32 val)
{
	pci_conf_write(pdev->pc, pdev->tag, reg, val);
	return 0;
} 

static inline int
pci_write_config_word(struct pci_dev *pdev, int reg, u16 val)
{
	uint32_t v;

	v = pci_conf_read(pdev->pc, pdev->tag, (reg & ~0x2));
	v &= ~(0xffff << ((reg & 0x2) * 8));
	v |= (val << ((reg & 0x2) * 8));
	pci_conf_write(pdev->pc, pdev->tag, (reg & ~0x2), v);
	return 0;
} 

static inline int
pci_write_config_byte(struct pci_dev *pdev, int reg, u8 val)
{
	uint32_t v;

	v = pci_conf_read(pdev->pc, pdev->tag, (reg & ~0x3));
	v &= ~(0xff << ((reg & 0x3) * 8));
	v |= (val << ((reg & 0x3) * 8));
	pci_conf_write(pdev->pc, pdev->tag, (reg & ~0x3), v);
	return 0;
}

static inline int
pci_bus_read_config_word(struct pci_bus *bus, unsigned int devfn,
    int reg, u16 *val)
{
	pcitag_t tag = pci_make_tag(bus->pc, bus->number,
	    PCI_SLOT(devfn), PCI_FUNC(devfn));
	uint32_t v;

	v = pci_conf_read(bus->pc, tag, (reg & ~0x2));
	*val = (v >> ((reg & 0x2) * 8));
	return 0;
}

static inline int
pci_bus_read_config_byte(struct pci_bus *bus, unsigned int devfn,
    int reg, u8 *val)
{
	pcitag_t tag = pci_make_tag(bus->pc, bus->number,
	    PCI_SLOT(devfn), PCI_FUNC(devfn));
	uint32_t v;

	v = pci_conf_read(bus->pc, tag, (reg & ~0x3));
	*val = (v >> ((reg & 0x3) * 8));
	return 0;
}

static inline int
pci_bus_write_config_byte(struct pci_bus *bus, unsigned int devfn,
    int reg, u8 val)
{
	pcitag_t tag = pci_make_tag(bus->pc, bus->number,
	    PCI_SLOT(devfn), PCI_FUNC(devfn));
	uint32_t v;

	v = pci_conf_read(bus->pc, tag, (reg & ~0x3));
	v &= ~(0xff << ((reg & 0x3) * 8));
	v |= (val << ((reg & 0x3) * 8));
	pci_conf_write(bus->pc, tag, (reg & ~0x3), v);
	return 0;
}

static inline int
pci_pcie_cap(struct pci_dev *pdev)
{
	int pos;
	if (!pci_get_capability(pdev->pc, pdev->tag, PCI_CAP_PCIEXPRESS,
	    &pos, NULL))
		return -EINVAL;
	return pos;
}

static inline bool
pci_is_root_bus(struct pci_bus *pbus)
{
	return (pbus->bridgetag == NULL);
}

static inline int
pcie_capability_read_dword(struct pci_dev *pdev, int off, u32 *val)
{
	int pos;
	if (!pci_get_capability(pdev->pc, pdev->tag, PCI_CAP_PCIEXPRESS,
	    &pos, NULL)) {
		*val = 0;
		return -EINVAL;
	}
	*val = pci_conf_read(pdev->pc, pdev->tag, pos + off);
	return 0;
}

#define pci_set_master(x)
#define pci_clear_master(x)

#define pci_save_state(x)
#define pci_restore_state(x)

#define pci_enable_msi(x)	0
#define pci_disable_msi(x)

typedef enum {
	PCI_D0,
	PCI_D1,
	PCI_D2,
	PCI_D3hot,
	PCI_D3cold
} pci_power_t;

enum pci_bus_speed {
	PCIE_SPEED_2_5GT,
	PCIE_SPEED_5_0GT,
	PCIE_SPEED_8_0GT,
	PCI_SPEED_UNKNOWN
};

enum pci_bus_speed pcie_get_speed_cap(struct pci_dev *);

#define pci_save_state(x)
#define pci_enable_device(x)		0
#define pci_disable_device(x)
#define pci_set_power_state(d, s)	0
#define pci_is_thunderbolt_attached(x) false
#define pci_set_drvdata(x, y)
#define dev_pm_set_driver_flags(x, y)

static inline int
vga_client_register(struct pci_dev *a, void *b, void *c, void *d)
{
	return -ENODEV;
}

#if defined(__amd64__) || defined(__i386__)

#define AGP_USER_MEMORY			0
#define AGP_USER_CACHED_MEMORY		BUS_DMA_COHERENT

#define PCI_DMA_BIDIRECTIONAL	0

static inline dma_addr_t
pci_map_page(struct pci_dev *pdev, struct vm_page *page, unsigned long offset, size_t size, int direction)
{
	return VM_PAGE_TO_PHYS(page);
}

static inline void
pci_unmap_page(struct pci_dev *pdev, dma_addr_t dma_address, size_t size, int direction)
{
}

static inline int
pci_dma_mapping_error(struct pci_dev *pdev, dma_addr_t dma_addr)
{
	return 0;
}

#define pci_set_dma_mask(x, y)			0
#define pci_set_consistent_dma_mask(x, y)	0

#define dma_set_coherent_mask(x, y)	0

#define VGA_RSRC_LEGACY_IO	0x01

void vga_get_uninterruptible(struct pci_dev *, int);
void vga_put(struct pci_dev *, int);

#endif

#define vga_switcheroo_register_client(a, b, c)	0
#define vga_switcheroo_unregister_client(a)
#define vga_switcheroo_process_delayed_switch()
#define vga_switcheroo_fini_domain_pm_ops(x)
#define vga_switcheroo_lock_ddc(x)
#define vga_switcheroo_unlock_ddc(x)
#define vga_switcheroo_handler_flags() 0

#define VGA_SWITCHEROO_CAN_SWITCH_DDC	1

#define memcpy_toio(d, s, n)	memcpy(d, s, n)
#define memcpy_fromio(d, s, n)	memcpy(d, s, n)
#define memset_io(d, b, n)	memset(d, b, n)

static inline u32
ioread32(const volatile void __iomem *addr)
{
	return (*(volatile uint32_t *)addr);
}

static inline u64
ioread64(const volatile void __iomem *addr)
{
	return (*(volatile uint64_t *)addr);
}

static inline void
iowrite32(u32 val, volatile void __iomem *addr)
{
	*(volatile uint32_t *)addr = val;
}

static inline void
iowrite64(u64 val, volatile void __iomem *addr)
{
	*(volatile uint64_t *)addr = val;
}

#define readl(p) ioread32(p)
#define writel(v, p) iowrite32(v, p)
#define readq(p) ioread64(p)
#define writeq(v, p) iowrite64(v, p)

#define page_to_phys(page)	(VM_PAGE_TO_PHYS(page))
#define page_to_pfn(pp)		(VM_PAGE_TO_PHYS(pp) / PAGE_SIZE)
#define pfn_to_page(pfn)	(PHYS_TO_VM_PAGE(ptoa(pfn)))
#define nth_page(page, n)	(&(page)[(n)])
#define offset_in_page(off)	((off) & PAGE_MASK)
#define set_page_dirty(page)	atomic_clearbits_int(&page->pg_flags, PG_CLEAN)

#define VERIFY_READ	0x1
#define VERIFY_WRITE	0x2
static inline int
access_ok(int type, const void *addr, unsigned long size)
{
	return true;
}

#define CAP_SYS_ADMIN	0x1
static inline int
capable(int cap)
{
	KASSERT(cap == CAP_SYS_ADMIN);
	return suser(curproc);
}

typedef unsigned long pgoff_t;
typedef int pgprot_t;
#define pgprot_val(v)	(v)
#define PAGE_KERNEL	0
#define PAGE_KERNEL_IO	0

static inline pgprot_t
pgprot_writecombine(pgprot_t prot)
{
#if PMAP_WC != 0
	return prot | PMAP_WC;
#else
	return prot | PMAP_NOCACHE;
#endif
}

static inline pgprot_t
pgprot_noncached(pgprot_t prot)
{
#if PMAP_DEVICE != 0
	return prot | PMAP_DEVICE;
#else
	return prot | PMAP_NOCACHE;
#endif
}

void	*kmap(struct vm_page *);
void	 kunmap(void *addr);
void	*vmap(struct vm_page **, unsigned int, unsigned long, pgprot_t);
void	 vunmap(void *, size_t);

#define is_vmalloc_addr(ptr)	true
#define kmap_to_page(ptr)	(ptr)

#define round_up(x, y) ((((x) + ((y) - 1)) / (y)) * (y))
#define round_down(x, y) (((x) / (y)) * (y)) /* y is power of two */
#define rounddown(x, y) (((x) / (y)) * (y)) /* arbitary y */
#define roundup2(x, y) (((x)+((y)-1))&(~((y)-1))) /* if y is powers of two */
#define DIV_ROUND_UP(x, y)	(((x) + ((y) - 1)) / (y))
#define DIV_ROUND_UP_ULL(x, y)	DIV_ROUND_UP(x, y)
#define DIV_ROUND_DOWN(x, y)	((x) / (y))
#define DIV_ROUND_DOWN_ULL(x, y)	DIV_ROUND_DOWN(x, y)
#define DIV_ROUND_CLOSEST(x, y)	(((x) + ((y) / 2)) / (y))
#define DIV_ROUND_CLOSEST_ULL(x, y)	DIV_ROUND_CLOSEST(x, y)

/*
 * Compute the greatest common divisor of a and b.
 * from libc getopt_long.c
 */
static inline unsigned long
gcd(unsigned long a, unsigned long b)
{
	unsigned long c;

	c = a % b;
	while (c != 0) {
		a = b;
		b = c;
		c = a % b;
	}

	return (b);
}

static inline unsigned long
roundup_pow_of_two(unsigned long x)
{
	return (1UL << flsl(x - 1));
}

static inline unsigned long
rounddown_pow_of_two(unsigned long x)
{
	return (1UL << (flsl(x) - 1));
}

static inline int
fls64(long long mask)
{
	int bit;

	if (mask == 0)
		return (0);
	for (bit = 1; mask != 1; bit++)
		mask = (unsigned long long)mask >> 1;
	return (bit);
}

#define is_power_of_2(x)	(((x) != 0) && (((x) - 1) & (x)) == 0)

#define PAGE_ALIGN(addr)	(((addr) + PAGE_MASK) & ~PAGE_MASK)
#define IS_ALIGNED(x, y)	(((x) & ((y) - 1)) == 0)

static inline void
udelay(unsigned long usecs)
{
	DELAY(usecs);
}

static inline void
ndelay(unsigned long nsecs)
{
	DELAY(max(nsecs / 1000, 1));
}

static inline void
usleep_range(unsigned long min, unsigned long max)
{
	DELAY(min);
}

static inline void
mdelay(unsigned long msecs)
{
	int loops = msecs;
	while (loops--)
		DELAY(1000);
}

static inline void
cpu_relax(void)
{
	CPU_BUSY_CYCLE();
	if (cold) {
		delay(tick);
		jiffies++;
	}
}

#define cpu_relax_lowlatency() CPU_BUSY_CYCLE()
#define cpu_has_pat	1
#define cpu_has_clflush	1

static inline uint32_t ror32(uint32_t word, unsigned int shift)
{
	return (word >> shift) | (word << (32 - shift));
}

static inline int
irqs_disabled(void)
{
	return (cold);
}

static inline int
in_dbg_master(void)
{
#ifdef DDB
	return (db_is_active);
#endif
	return (0);
}

#define oops_in_progress in_dbg_master()

static inline int
power_supply_is_system_supplied(void)
{
	/* XXX return 0 if on battery */
	return (1);
}

#define pm_qos_update_request(x, y)
#define pm_qos_remove_request(x)
#define pm_runtime_mark_last_busy(x)
#define pm_runtime_use_autosuspend(x)
#define pm_runtime_put_autosuspend(x)
#define pm_runtime_set_autosuspend_delay(x, y)
#define pm_runtime_set_active(x)
#define pm_runtime_allow(x)
#define pm_runtime_put_noidle(x)

static inline int
pm_runtime_get_sync(struct device *dev)
{
	return 0;
}

#define _U      0x01
#define _L      0x02
#define _N      0x04
#define _S      0x08
#define _P      0x10
#define _C      0x20
#define _X      0x40
#define _B      0x80

static inline int
isascii(int c)
{
	return ((unsigned int)c <= 0177);
}

static inline int
isprint(int c)
{
	if (c == -1)
		return (0);
	if ((unsigned char)c >= 040 && (unsigned char)c <= 0176)
		return (1);
	return (0);
}

#ifdef __macppc__
static inline int
of_machine_is_compatible(const char *model)
{
	extern char *hw_prod;
	return (strcmp(model, hw_prod) == 0);
}
#endif

typedef unsigned int gfp_t;

#define MAX_ORDER	11

struct vm_page *alloc_pages(unsigned int, unsigned int);
void	__free_pages(struct vm_page *, unsigned int);

static inline struct vm_page *
alloc_page(unsigned int gfp_mask)
{
	return alloc_pages(gfp_mask, 0);
}

static inline void
__free_page(struct vm_page *page)
{
	return __free_pages(page, 0);
}

static inline unsigned int
get_order(size_t size)
{
	return flsl((size - 1) >> PAGE_SHIFT);
}

#define ilog2(x) ((sizeof(x) <= 4) ? (fls(x) - 1) : (flsl(x) - 1))

#if defined(__i386__) || defined(__amd64__)

#define _PAGE_PRESENT	PG_V
#define _PAGE_RW	PG_RW
#define _PAGE_PAT	PG_PAT
#define _PAGE_PWT	PG_WT
#define _PAGE_PCD	PG_N

static inline void
pagefault_disable(void)
{
	KASSERT(curcpu()->ci_inatomic == 0);
	curcpu()->ci_inatomic = 1;
}

static inline void
pagefault_enable(void)
{
	KASSERT(curcpu()->ci_inatomic == 1);
	curcpu()->ci_inatomic = 0;
}

static inline int
pagefault_disabled(void)
{
	return curcpu()->ci_inatomic;
}

static inline void *
kmap_atomic(struct vm_page *pg)
{
	vaddr_t va;

#if defined (__HAVE_PMAP_DIRECT)
	va = pmap_map_direct(pg);
#else
	extern vaddr_t pmap_tmpmap_pa(paddr_t);
	va = pmap_tmpmap_pa(VM_PAGE_TO_PHYS(pg));
#endif
	return (void *)va;
}

static inline void
kunmap_atomic(void *addr)
{
#if defined (__HAVE_PMAP_DIRECT)
	pmap_unmap_direct((vaddr_t)addr);
#else
	extern void pmap_tmpunmap_pa(void);
	pmap_tmpunmap_pa();
#endif
}

static inline unsigned long
__copy_to_user_inatomic(void *to, const void *from, unsigned len)
{
	struct cpu_info *ci = curcpu();
	int inatomic = ci->ci_inatomic;
	int error;

	ci->ci_inatomic = 1;
	error = copyout(from, to, len);
	ci->ci_inatomic = inatomic;

	return (error ? len : 0);
}

static inline unsigned long
__copy_from_user_inatomic(void *to, const void *from, unsigned len)
{
	struct cpu_info *ci = curcpu();
	int inatomic = ci->ci_inatomic;
	int error;

	ci->ci_inatomic = 1;
	error = copyin(from, to, len);
	ci->ci_inatomic = inatomic;

	return (error ? len : 0);
}

static inline unsigned long
__copy_from_user_inatomic_nocache(void *to, const void *from, unsigned len)
{
	return __copy_from_user_inatomic(to, from, len);
}

#endif

struct fb_var_screeninfo {
	int pixclock;
	uint32_t width;
	uint32_t height;
};

struct fb_info {
	struct fb_var_screeninfo var;
	char *screen_buffer;
	void *par;
	int fbcon_rotate_hint;
};

#define FB_BLANK_UNBLANK	0
#define FB_BLANK_NORMAL		1
#define FB_BLANK_HSYNC_SUSPEND	2
#define FB_BLANK_VSYNC_SUSPEND	3
#define FB_BLANK_POWERDOWN	4

#define FBINFO_STATE_RUNNING	0
#define FBINFO_STATE_SUSPENDED	1

#define FB_ROTATE_UR		0
#define FB_ROTATE_CW		1
#define FB_ROTATE_UD		2
#define FB_ROTATE_CCW		3

#define framebuffer_alloc(flags, device) \
	kzalloc(sizeof(struct fb_info), GFP_KERNEL)

#define fb_set_suspend(x, y)

struct address_space;
#define unmap_mapping_range(mapping, holebegin, holeend, even_cows)

/*
 * ACPI types and interfaces.
 */

typedef size_t acpi_size;
typedef int acpi_status;

struct acpi_table_header;

#define ACPI_SUCCESS(x) ((x) == 0)

#define AE_NOT_FOUND	0x0005

acpi_status acpi_get_table(const char *, int, struct acpi_table_header **);

#define acpi_video_register()
#define acpi_video_unregister()

struct backlight_device;

struct backlight_properties {
	int type;
	int max_brightness;
	int brightness;
	int power;
};

struct backlight_ops {
	int (*update_status)(struct backlight_device *);
	int (*get_brightness)(struct backlight_device *);
};

struct backlight_device {
	const struct backlight_ops *ops;
	struct backlight_properties props;
	struct task task;
	void *data;
};

#define bl_get_data(bd)	(bd)->data

#define BACKLIGHT_RAW		0
#define BACKLIGHT_FIRMWARE	1

struct backlight_device *backlight_device_register(const char *, void *,
     void *, const struct backlight_ops *, struct backlight_properties *);
void backlight_device_unregister(struct backlight_device *);

static inline void
backlight_update_status(struct backlight_device *bd)
{
	bd->ops->update_status(bd);
}

void backlight_schedule_update_status(struct backlight_device *);

struct pwm_device;

static inline struct pwm_device *
pwm_get(struct device *dev, const char *consumer)
{
	return ERR_PTR(-ENODEV);
}

static inline void
pwm_put(struct pwm_device *pwm)
{
}

static inline unsigned int
pwm_get_duty_cycle(const struct pwm_device *pwm)
{
	return 0;
}

static inline int
pwm_config(struct pwm_device *pwm, int duty_ns, int period_ns)
{
	return -EINVAL;
}

static inline int
pwm_enable(struct pwm_device *pwm)
{
	return -EINVAL;
}

static inline void
pwm_disable(struct pwm_device *pwm)
{
}

static inline void
pwm_apply_args(struct pwm_device *pwm)
{
}

struct scatterlist {
	dma_addr_t dma_address;
	unsigned int offset;
	unsigned int length;
	bool end;
};

struct sg_table {
	struct scatterlist *sgl;
	unsigned int nents;
	unsigned int orig_nents;
};

struct sg_page_iter {
	struct scatterlist *sg;
	unsigned int sg_pgoffset;
	unsigned int __nents;
};

#define sg_is_chain(sg)		false
#define sg_is_last(sg)		((sg)->end)
#define sg_chain_ptr(sg)	NULL

int sg_alloc_table(struct sg_table *, unsigned int, gfp_t);
void sg_free_table(struct sg_table *);

static inline void
sg_mark_end(struct scatterlist *sgl)
{
	sgl->end = true;
}

static inline void
__sg_page_iter_start(struct sg_page_iter *iter, struct scatterlist *sgl,
    unsigned int nents, unsigned long pgoffset)
{
	iter->sg = sgl;
	iter->sg_pgoffset = pgoffset - 1;
	iter->__nents = nents;
}

static inline bool
__sg_page_iter_next(struct sg_page_iter *iter)
{
	iter->sg_pgoffset++;
	while (iter->__nents > 0 && 
	    iter->sg_pgoffset >= (iter->sg->length / PAGE_SIZE)) {
		iter->sg_pgoffset -= (iter->sg->length / PAGE_SIZE);
		iter->sg++;
		iter->__nents--;
	}

	return (iter->__nents > 0);
}

static inline paddr_t
sg_page_iter_dma_address(struct sg_page_iter *iter)
{
	return iter->sg->dma_address + (iter->sg_pgoffset << PAGE_SHIFT);
}

static inline struct vm_page *
sg_page_iter_page(struct sg_page_iter *iter)
{
	return PHYS_TO_VM_PAGE(sg_page_iter_dma_address(iter));
}

static inline struct vm_page *
sg_page(struct scatterlist *sgl)
{
	return PHYS_TO_VM_PAGE(sgl->dma_address);
}

static inline void
sg_set_page(struct scatterlist *sgl, struct vm_page *page,
    unsigned int length, unsigned int offset)
{
	sgl->dma_address = VM_PAGE_TO_PHYS(page);
	sgl->offset = offset;
	sgl->length = length;
	sgl->end = false;
}

#define sg_dma_address(sg)	((sg)->dma_address)
#define sg_dma_len(sg)		((sg)->length)

#define for_each_sg_page(sgl, iter, nents, pgoffset) \
  __sg_page_iter_start((iter), (sgl), (nents), (pgoffset)); \
  while (__sg_page_iter_next(iter))

size_t sg_copy_from_buffer(struct scatterlist *, unsigned int,
    const void *, size_t);

struct firmware {
	size_t size;
	const u8 *data;
};

static inline int
request_firmware(const struct firmware **fw, const char *name,
    struct device *device)
{
	int r;
	struct firmware *f = malloc(sizeof(struct firmware), M_DRM,
	    M_WAITOK | M_ZERO);
	*fw = f;
	r = loadfirmware(name, __DECONST(u_char **, &f->data), &f->size);
	if (r != 0)
		return -r;
	else
		return 0;
}

#define request_firmware_nowait(a, b, c, d, e, f, g) -EINVAL

static inline void
release_firmware(const struct firmware *fw)
{
	if (fw)
		free(__DECONST(u_char *, fw->data), M_DEVBUF, fw->size);
	free(__DECONST(struct firmware *, fw), M_DRM, sizeof(*fw));
}

void *memchr_inv(const void *, int, size_t);

struct dma_buf_ops;

struct dma_buf {
	const struct dma_buf_ops *ops;
	void *priv;
	size_t size;
	struct file *file;
};

struct dma_buf_attachment;

void	get_dma_buf(struct dma_buf *);
struct dma_buf *dma_buf_get(int);
void	dma_buf_put(struct dma_buf *);
int	dma_buf_fd(struct dma_buf *, int);

struct dma_buf_ops {
	void (*release)(struct dma_buf *);
};

struct dma_buf_export_info {
	const struct dma_buf_ops *ops;
	size_t size;
	int flags;
	void *priv;
	struct reservation_object *resv;
};

#define DEFINE_DMA_BUF_EXPORT_INFO(x)  struct dma_buf_export_info x 

struct dma_buf *dma_buf_export(const struct dma_buf_export_info *);

#define dma_buf_attach(x, y) NULL
#define dma_buf_detach(x, y) panic("dma_buf_detach")

#define register_sysrq_key(x, y)

struct pmu {
};

#define NOTIFY_DONE	0
#define NOTIFY_OK	1
#define NOTIFY_BAD	2

#define might_sleep()
#define might_sleep_if(x)
#define get_random_u32()	arc4random()
#define get_random_int()	arc4random()

static inline uint64_t
get_random_u64(void)
{
	uint64_t r;
	arc4random_buf(&r, sizeof(r));
	return r;
}

static inline unsigned long
get_random_long(void)
{
#ifdef __LP64__
	return get_random_u64();
#else
	return get_random_u32();
#endif
}

#define add_taint(x, y)
#define TAINT_MACHINE_CHECK	0
#define LOCKDEP_STILL_OK	0

#define smp_processor_id()	(curcpu()->ci_cpuid)

#define ___stringify(x...)	#x
#define __stringify(x...)	___stringify(x)

#define sysfs_create_link(x, y, z)	0
#define sysfs_remove_link(x, y)
#define sysfs_create_group(x, y)	0
#define sysfs_remove_group(x, y)

static inline void *
memset32(uint32_t *b, uint32_t c, size_t len)
{
	uint32_t *dst = b;
	while (len--)
		*dst++ = c;
	return b;
}

static inline void *
memset64(uint64_t *b, uint64_t c, size_t len)
{
	uint64_t *dst = b;
	while (len--)
		*dst++ = c;
	return b;
}

static inline void *
memset_p(void **p, void *v, size_t n)
{
#ifdef __LP64__
	return memset64((uint64_t *)p, (uintptr_t)v, n);
#else
	return memset32((uint32_t *)p, (uintptr_t)v, n);
#endif
}

#define POISON_INUSE	0xdb

#define cec_notifier_set_phys_addr_from_edid(x, y)
#define cec_notifier_phys_addr_invalidate(x)
#define cec_notifier_put(x)
#define cec_notifier_get_conn(x, y)			NULL

#define SZ_4K	(1024 * 4)
#define SZ_8K	(1024 * 8)
#define SZ_32K	(1024 * 32)
#define SZ_128K	(1024 * 128)
#define SZ_1M	(1024 * 1024)
#define SZ_16M	(16 * 1024 * 1024)

#define typecheck(x, y)		1

enum hrtimer_restart { HRTIMER_NORESTART, HRTIMER_RESTART };
struct hrtimer {
	enum hrtimer_restart	(*function)(struct hrtimer *);
};

#define HRTIMER_MODE_REL	1

#define hrtimer_cancel(x)	timeout_del(x)
#define hrtimer_active(x)	timeout_pending(x)

#define MBI_PMIC_BUS_ACCESS_BEGIN	1
#define MBI_PMIC_BUS_ACCESS_END		2

#define iosf_mbi_assert_punit_acquired()
#define iosf_mbi_punit_acquire()
#define iosf_mbi_punit_release()
#define iosf_mbi_register_pmic_bus_access_notifier(x)			0
#define iosf_mbi_unregister_pmic_bus_access_notifier_unlocked(x)	0

static inline int
match_string(const char * const *array,  size_t n, const char *str)
{
	int i;

	for (i = 0; i < n; i++) {
		if (array[i] == NULL)
			break;
		if (!strcmp(array[i], str))	
			return i;
	}

	return -EINVAL;
}

#define UUID_STRING_LEN 36

#define PAGEVEC_SIZE 15

struct pagevec {
	uint8_t	nr;
	struct vm_page *pages[PAGEVEC_SIZE];
};

static inline unsigned int
pagevec_space(struct pagevec *pvec)
{
	return PAGEVEC_SIZE - pvec->nr;
}

static inline void
pagevec_init(struct pagevec *pvec)
{
	pvec->nr = 0;
}

static inline void
pagevec_reinit(struct pagevec *pvec)
{
	pvec->nr = 0;
}

static inline unsigned int
pagevec_count(struct pagevec *pvec)
{
	return pvec->nr;
}

static inline void
__pagevec_release(struct pagevec *pvec)
{
	STUB();
}

static inline unsigned int
pagevec_add(struct pagevec *pvec, struct vm_page *page)
{
	STUB();
	return -ENOSYS;
}

#define page_address(x)	VM_PAGE_TO_PHYS(x)

#if defined(__amd64__) || defined(__i386__)

static inline int
set_pages_array_wb(struct vm_page **pages, int addrinarray)
{
	int i;

	for (i = 0; i < addrinarray; i++)
		atomic_clearbits_int(&pages[i]->pg_flags, PG_PMAP_WC);

	return 0;
}

static inline int
set_pages_array_wc(struct vm_page **pages, int addrinarray)
{
	int i;

	for (i = 0; i < addrinarray; i++)
		atomic_setbits_int(&pages[i]->pg_flags, PG_PMAP_WC);

	return 0;
}

static inline int
set_pages_array_uc(struct vm_page **pages, int addrinarray)
{
	/* XXX */
	return 0;
}

static inline int
set_pages_wb(struct vm_page *page, int numpages)
{
	KASSERT(numpages == 1);
	atomic_clearbits_int(&page->pg_flags, PG_PMAP_WC);
	return 0;
}

static inline int
set_pages_uc(struct vm_page *page, int numpages)
{
	/* XXX */
	return 0;
}

#endif

typedef int (*cpu_stop_fn_t)(void *arg);

static inline int
stop_machine(cpu_stop_fn_t fn, void *arg, void *cpus)
{
	int r;
	intr_disable();
	r = (*fn)(arg);	
	intr_enable();
	return r;
}

#define put_pid(x)

static inline int
get_unused_fd_flags(unsigned int flags)
{
	STUB();
	return -1;
}

#if defined(__amd64__) || defined(__i386__)

#define X86_FEATURE_CLFLUSH	1
#define X86_FEATURE_XMM4_1	2
#define X86_FEATURE_PAT		3
#define X86_FEATURE_HYPERVISOR	4

static inline bool
static_cpu_has(uint16_t f)
{
	switch (f) {
	case X86_FEATURE_CLFLUSH:
		return curcpu()->ci_cflushsz != 0;
	case X86_FEATURE_XMM4_1:
		return (cpu_ecxfeature & CPUIDECX_SSE41) != 0;
	case X86_FEATURE_PAT:
		return (curcpu()->ci_feature_flags & CPUID_PAT) != 0;
	case X86_FEATURE_HYPERVISOR:
		return (cpu_ecxfeature & CPUIDECX_HV) != 0;
	default:
		return false;
	}
}

#define boot_cpu_has(x) static_cpu_has(x)

#endif

#define kthread_stop(a)			STUB()

#define prefetchw(x)	__builtin_prefetch(x,1)

static inline long
get_nr_swap_pages(void)
{
	return uvmexp.swpages - uvmexp.swpginuse;
}

static inline long
si_mem_available(void)
{
	return uvmexp.free;
}

#define array_index_nospec(a, b)	(a)

static inline int
wake_up_process(struct proc *p)
{
	int s, r = 0;

	SCHED_LOCK(s);
	if (p->p_stat == SSLEEP) {
		setrunnable(p);
		r = 1;
	}
	SCHED_UNLOCK(s);
	
	return r;
}

#endif
