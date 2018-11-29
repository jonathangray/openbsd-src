/* Public domain. */

#ifndef _LINUX_IRQ_H
#define _LINUX_IRQ_H

typedef int irqreturn_t;
enum irqreturn {
	IRQ_NONE = 0,
	IRQ_HANDLED = 1
};

#endif
