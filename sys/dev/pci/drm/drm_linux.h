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
#include <linux/printk.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/firmware.h>
#include <linux/interrupt.h>
#include <linux/hash.h>
#include <linux/fb.h>
#include <linux/vga_switcheroo.h>
#include <linux/notifier.h>
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

#define CONFIG_DRM_FBDEV_OVERALLOC	0
#define CONFIG_DRM_I915_DEBUG		0
#define CONFIG_DRM_I915_DEBUG_GEM	0
#define CONFIG_PM			0

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

#define DMA_BIT_MASK(n) (((n) == 64) ? ~0ULL : (1ULL<<(n)) -1)
#define GENMASK(h, l)		(((~0UL) >> (BITS_PER_LONG - (h) - 1)) & ((~0UL) << (l)))
#define GENMASK_ULL(h, l)	(((~0ULL) >> (BITS_PER_LONG_LONG - (h) - 1)) & ((~0ULL) << (l)))

#define KERN_INFO	""
#define KERN_WARNING	""
#define KERN_NOTICE	""
#define KERN_DEBUG	""
#define KERN_CRIT	""
#define KERN_ERR	""

#define KBUILD_MODNAME "drm"

#define UTS_RELEASE	""

#define DEFINE_RATELIMIT_STATE(name, interval, burst) \
	int name __used = 1;

#define __ratelimit(x)	(1)

#define local_irq_save(x)		(x) = splhigh()
#define local_irq_restore(x)		splx((x))
#define synchronize_irq(x)

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

#define array_size(x, y) ((x) * (y))

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

static inline bool
refcount_dec_and_test(uint32_t *p)
{
	return atomic_dec_and_test(p);
}

#define register_reboot_notifier(x)
#define unregister_reboot_notifier(x)

#define SYS_RESTART 0

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

#define VERIFY_READ	0x1
#define VERIFY_WRITE	0x2
static inline int
access_ok(int type, const void *addr, unsigned long size)
{
	return true;
}

#define is_vmalloc_addr(ptr)	true

#define roundup2(x, y) (((x)+((y)-1))&(~((y)-1))) /* if y is powers of two */

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

#define MAX_ORDER	11

static inline unsigned int
get_order(size_t size)
{
	return flsl((size - 1) >> PAGE_SHIFT);
}

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

#define page_address(x)	VM_PAGE_TO_PHYS(x)

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
