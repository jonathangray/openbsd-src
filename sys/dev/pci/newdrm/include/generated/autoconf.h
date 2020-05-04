/* Public domain. */

#include <sys/param.h>

#define CONFIG_BACKLIGHT_CLASS_DEVICE		1
#define CONFIG_DRM_FBDEV_EMULATION		1
#define CONFIG_DRM_FBDEV_OVERALLOC		0
#define CONFIG_DRM_PANEL			1
#define CONFIG_DRM_I915_DEBUG			0
#define CONFIG_DRM_I915_DEBUG_GEM		0
#define CONFIG_DRM_I915_FBDEV			1
#define CONFIG_DRM_I915_ALPHA_SUPPORT		0
#define CONFIG_DRM_I915_CAPTURE_ERROR		1
#define CONFIG_DRM_I915_GVT			0
#define CONFIG_DRM_I915_SW_FENCE_CHECK_DAG	0
#define CONFIG_PM				0
#define CONFIG_DRM_AMD_DC			1
#if defined(__amd64__) || defined(__i386__)
#define CONFIG_DRM_AMD_DC_DCN			1
#endif
#if 0
#define CONFIG_DRM_AMDGPU_CIK			1
#define CONFIG_DRM_AMDGPU_SI			1
#endif

#define CONFIG_DRM_I915_PREEMPT_TIMEOUT		640	/* milliseconds */
#define CONFIG_DRM_I915_TIMESLICE_DURATION	1	/* milliseconds */
#define CONFIG_DRM_I915_FORCE_PROBE		""

#ifdef __HAVE_ACPI
#include "acpi.h"
#if NACPI > 0
#define CONFIG_ACPI				1
#endif
#endif

#include "pci.h"
#if NPCI > 0
#define CONFIG_PCI				1
#endif

#include "agp.h"
#if NAGP > 0
#define CONFIG_AGP				1
#endif

#if defined(__amd64__) || defined(__i386__)
#define CONFIG_DMI				1
#endif

#ifdef __amd64__
#define CONFIG_X86				1
#define CONFIG_X86_64				1
#define CONFIG_X86_PAT				1
#endif

#ifdef __i386__
#define CONFIG_X86				1
#define CONFIG_X86_32				1
#define CONFIG_X86_PAT				1
#endif
