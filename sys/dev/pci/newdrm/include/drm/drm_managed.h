/* Public domain. */

#ifndef _DRM_MANAGED_H_
#define _DRM_MANAGED_H_

static inline void *
drmm_kzalloc(struct drm_device *dev, size_t size, int flags)
{
	STUB();
	return kzalloc(size, flags);
}

static inline void *
drmm_kcalloc(struct drm_device *dev, size_t n, size_t size, int flags)
{
	STUB();
	return kcalloc(n, size, flags);
}

static inline char *
drmm_kstrdup(struct drm_device *dev, const char *s, int flags)
{
	STUB();
	return kstrdup(s, flags);
}

static inline void
drmm_kfree(struct drm_device *dev, void *p)
{
	STUB();
}

static inline int
drmm_add_action(struct drm_device *dev, void *f, void *p)
{
	STUB();
	return 0;
}

static inline int
drmm_add_action_or_reset(struct drm_device *dev, void *f, void *p)
{
	STUB();
	return 0;
}

#endif
