/*	$OpenBSD: drm_linux.h,v 1.93 2019/04/14 10:14:51 jsg Exp $	*/
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

#include <dev/pci/pcivar.h>

#include <linux/kconfig.h>

#define KBUILD_MODNAME "drm"

#define KHZ2PICOS(a)	(1000000000UL/(a))

#ifndef PCI_MEM_START
#define PCI_MEM_START	0
#endif

#ifndef PCI_MEM_END
#define PCI_MEM_END	0xffffffff
#endif

#ifndef PCI_MEM64_END
#define PCI_MEM64_END	0xffffffffffffffff
#endif

#ifdef __macppc__
static inline int
of_machine_is_compatible(const char *model)
{
	extern char *hw_prod;
	return (strcmp(model, hw_prod) == 0);
}
#endif

#define POISON_INUSE	0xdb

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

#endif
