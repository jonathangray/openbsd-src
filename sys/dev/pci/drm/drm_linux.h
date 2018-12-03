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
#include <sys/kernel.h>
#include <sys/stdint.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

#include <dev/pci/pcivar.h>

#include <linux/types.h>

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

#define KBUILD_MODNAME "drm"

#define KHZ2PICOS(a)	(1000000000UL/(a))

#define order_base_2(x) drm_order(x)

#define u64_to_user_ptr(x)	((void *)(uintptr_t)(x))

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

#define cpu_relax_lowlatency() CPU_BUSY_CYCLE()
#define cpu_has_pat	1
#define cpu_has_clflush	1

#ifdef __macppc__
static inline int
of_machine_is_compatible(const char *model)
{
	extern char *hw_prod;
	return (strcmp(model, hw_prod) == 0);
}
#endif

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

#define smp_processor_id()	(curcpu()->ci_cpuid)

#define sysfs_create_link(x, y, z)	0
#define sysfs_remove_link(x, y)
#define sysfs_create_group(x, y)	0
#define sysfs_remove_group(x, y)

#define POISON_INUSE	0xdb

#define typecheck(x, y)		1

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
