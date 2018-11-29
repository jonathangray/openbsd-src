/* Public domain. */

#ifndef _LINUX_DEVICE_H
#define _LINUX_DEVICE_H

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <linux/slab.h>
#include <linux/ioport.h>

struct device_node;

struct device_driver {
	struct device *dev;
};

#define dev_get_drvdata(x)	NULL
#define dev_set_drvdata(x, y)
#define dev_name(dev)		""

#define devm_kzalloc(x, y, z)	kzalloc(y, z)

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

#endif
