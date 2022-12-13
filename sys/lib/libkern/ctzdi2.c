/*	$OpenBSD$	*/
/* Public domain. */

#include <lib/libkern/libkern.h>

int
__ctzdi2(long mask)
{
	/* mask 0 is undefined */
	return ffsl(mask) - 1;
}
