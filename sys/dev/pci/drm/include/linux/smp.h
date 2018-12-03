/* Public domain. */

#ifndef _LINUX_SMP_H
#define _LINUX_SMP_H

#include <sys/srp.h>
#include <machine/cpu.h>

#define smp_processor_id()	(curcpu()->ci_cpuid)

#endif
