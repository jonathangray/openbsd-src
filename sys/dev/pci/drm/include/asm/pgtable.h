/* Public domain. */

#ifndef _ASM_PGTABLE_H
#define _ASM_PGTABLE_H

#include <machine/pmap.h>
#include <linux/types.h>

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

#endif
