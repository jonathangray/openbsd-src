/* Public domain. */

#include <drm/drm_device.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>

void *
drmm_kzalloc(struct drm_device *dev, size_t size, int flags)
{
	STUB();
	return kzalloc(size, flags);
}

void *
drmm_kcalloc(struct drm_device *dev, size_t n, size_t size, int flags)
{
	STUB();
	return kcalloc(n, size, flags);
}

char *
drmm_kstrdup(struct drm_device *dev, const char *s, int flags)
{
	STUB();
	return kstrdup(s, flags);
}

void
drmm_kfree(struct drm_device *dev, void *p)
{
	STUB();
}

int
drmm_add_action(struct drm_device *dev, void *f, void *p)
{
	STUB();
	return 0;
}

int
drmm_add_action_or_reset(struct drm_device *dev, void *f, void *p)
{
	STUB();
	return 0;
}

void
drm_managed_release(struct drm_device *dev)
{
	STUB();
}

void
drmm_add_final_kfree(struct drm_device *dev, void *p)
{
	STUB();
	dev->managed.final_kfree = p;
}
