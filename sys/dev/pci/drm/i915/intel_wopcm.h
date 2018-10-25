/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright © 2017-2018 Intel Corporation
 */

#ifndef _INTEL_WOPCM_H_
#define _INTEL_WOPCM_H_

#ifdef __linux__
#include <linux/types.h>
#else
#include <dev/pci/drm/drm_linux.h>
#endif

/**
 * struct intel_wopcm - Overall WOPCM info and WOPCM regions.
 * @size: Size of overall WOPCM.
 * @guc: GuC WOPCM Region info.
 * @guc.base: GuC WOPCM base which is offset from WOPCM base.
 * @guc.size: Size of the GuC WOPCM region.
 */
struct intel_wopcm {
	u32 size;
	struct {
		u32 base;
		u32 size;
	} guc;
};

void intel_wopcm_init_early(struct intel_wopcm *wopcm);
int intel_wopcm_init(struct intel_wopcm *wopcm);
int intel_wopcm_init_hw(struct intel_wopcm *wopcm);

#endif
