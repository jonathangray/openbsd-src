/* Public domain. */

#ifndef _LINUX_WAIT_BIT_H
#define _LINUX_WAIT_BIT_H

#include <linux/wait.h>

int wait_on_bit(unsigned long *, int, unsigned);
int wait_on_bit_timeout(unsigned long *, int, unsigned, int);
void wake_up_bit(void *, int);
wait_queue_head_t *bit_waitqueue(void *, int);

#endif
