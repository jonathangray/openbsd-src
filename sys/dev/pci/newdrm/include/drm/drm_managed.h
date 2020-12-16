/* Public domain. */

#ifndef _DRM_MANAGED_H_
#define _DRM_MANAGED_H_

static inline void *
drmm_kzalloc(struct drm_device *dev, size_t size, int flags)
{
	panic("unimplemented");
}

static inline void *
drmm_kcalloc(struct drm_device *dev, size_t n, size_t size, int flags)
{
	panic("unimplemented");
}

static inline char *
drmm_kstrdup(struct drm_device *dev, const char *s, int flags)
{
	panic("unimplemented");
}

static inline void
drmm_kfree(struct drm_device *dev, void *p)
{
	panic("unimplemented");
}

static inline int
drmm_add_action(struct drm_device *dev, void *f, void *p)
{
	panic("unimplemented");
}

static inline int
drmm_add_action_or_reset(struct drm_device *dev, void *f, void *p)
{
	panic("unimplemented");
}

#endif
