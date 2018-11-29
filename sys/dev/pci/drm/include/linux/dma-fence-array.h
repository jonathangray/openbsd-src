/* Public domain. */

#ifndef _LINUX_DMA_FENCE_ARRAY_H
#define _LINUX_DMA_FENCE_ARRAY_H

#include <linux/dma-fence.h>

struct dma_fence_array {
	unsigned int num_fences;
	struct dma_fence **fences;
};

static inline struct dma_fence_array *
to_dma_fence_array(struct dma_fence *fence)
{
	return NULL;
}

static inline bool
dma_fence_is_array(struct dma_fence *fence)
{
	return false;
}

#endif
