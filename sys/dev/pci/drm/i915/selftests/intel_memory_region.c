// SPDX-License-Identifier: MIT
/*
 * Copyright © 2019 Intel Corporation
 */

#include <linux/prime_numbers.h>
#include <linux/sort.h>

#include "../i915_selftest.h"

#include "mock_drm.h"
#include "mock_gem_device.h"
#include "mock_region.h"

#include "gem/i915_gem_context.h"
#include "gem/i915_gem_lmem.h"
#include "gem/i915_gem_region.h"
#include "gem/i915_gem_object_blt.h"
#include "gem/selftests/igt_gem_utils.h"
#include "gem/selftests/mock_context.h"
#include "gt/intel_engine_user.h"
#include "gt/intel_gt.h"
#include "i915_memcpy.h"
#include "selftests/igt_flush_test.h"
#include "selftests/i915_random.h"

static void close_objects(struct intel_memory_region *mem,
			  struct list_head *objects)
{
	struct drm_i915_private *i915 = mem->i915;
	struct drm_i915_gem_object *obj, *on;

	list_for_each_entry_safe(obj, on, objects, st_link) {
		if (i915_gem_object_has_pinned_pages(obj))
			i915_gem_object_unpin_pages(obj);
		/* No polluting the memory region between tests */
		__i915_gem_object_put_pages(obj);
		list_del(&obj->st_link);
		i915_gem_object_put(obj);
	}

	cond_resched();

	i915_gem_drain_freed_objects(i915);
}

static int igt_mock_fill(void *arg)
{
	struct intel_memory_region *mem = arg;
	resource_size_t total = resource_size(&mem->region);
	resource_size_t page_size;
	resource_size_t rem;
	unsigned long max_pages;
	unsigned long page_num;
	DRM_LIST_HEAD(objects);
	int err = 0;

	page_size = mem->mm.chunk_size;
	max_pages = div64_u64(total, page_size);
	rem = total;

	for_each_prime_number_from(page_num, 1, max_pages) {
		resource_size_t size = page_num * page_size;
		struct drm_i915_gem_object *obj;

		obj = i915_gem_object_create_region(mem, size, 0);
		if (IS_ERR(obj)) {
			err = PTR_ERR(obj);
			break;
		}

		err = i915_gem_object_pin_pages(obj);
		if (err) {
			i915_gem_object_put(obj);
			break;
		}

		list_add(&obj->st_link, &objects);
		rem -= size;
	}

	if (err == -ENOMEM)
		err = 0;
	if (err == -ENXIO) {
		if (page_num * page_size <= rem) {
			pr_err("%s failed, space still left in region\n",
			       __func__);
			err = -EINVAL;
		} else {
			err = 0;
		}
	}

	close_objects(mem, &objects);

	return err;
}

static struct drm_i915_gem_object *
igt_object_create(struct intel_memory_region *mem,
		  struct list_head *objects,
		  u64 size,
		  unsigned int flags)
{
	struct drm_i915_gem_object *obj;
	int err;

	obj = i915_gem_object_create_region(mem, size, flags);
	if (IS_ERR(obj))
		return obj;

	err = i915_gem_object_pin_pages(obj);
	if (err)
		goto put;

	list_add(&obj->st_link, objects);
	return obj;

put:
	i915_gem_object_put(obj);
	return ERR_PTR(err);
}

static void igt_object_release(struct drm_i915_gem_object *obj)
{
	i915_gem_object_unpin_pages(obj);
	__i915_gem_object_put_pages(obj);
	list_del(&obj->st_link);
	i915_gem_object_put(obj);
}

static int igt_mock_contiguous(void *arg)
{
	struct intel_memory_region *mem = arg;
	struct drm_i915_gem_object *obj;
	unsigned long n_objects;
	DRM_LIST_HEAD(objects);
	DRM_LIST_HEAD(holes);
	I915_RND_STATE(prng);
	resource_size_t total;
	resource_size_t min;
	u64 target;
	int err = 0;

	total = resource_size(&mem->region);

	/* Min size */
	obj = igt_object_create(mem, &objects, mem->mm.chunk_size,
				I915_BO_ALLOC_CONTIGUOUS);
	if (IS_ERR(obj))
		return PTR_ERR(obj);

	if (obj->mm.pages->nents != 1) {
		pr_err("%s min object spans multiple sg entries\n", __func__);
		err = -EINVAL;
		goto err_close_objects;
	}

	igt_object_release(obj);

	/* Max size */
	obj = igt_object_create(mem, &objects, total, I915_BO_ALLOC_CONTIGUOUS);
	if (IS_ERR(obj))
		return PTR_ERR(obj);

	if (obj->mm.pages->nents != 1) {
		pr_err("%s max object spans multiple sg entries\n", __func__);
		err = -EINVAL;
		goto err_close_objects;
	}

	igt_object_release(obj);

	/* Internal fragmentation should not bleed into the object size */
	target = i915_prandom_u64_state(&prng);
	div64_u64_rem(target, total, &target);
	target = round_up(target, PAGE_SIZE);
	target = max_t(u64, PAGE_SIZE, target);

	obj = igt_object_create(mem, &objects, target,
				I915_BO_ALLOC_CONTIGUOUS);
	if (IS_ERR(obj))
		return PTR_ERR(obj);

	if (obj->base.size != target) {
		pr_err("%s obj->base.size(%zx) != target(%llx)\n", __func__,
		       obj->base.size, target);
		err = -EINVAL;
		goto err_close_objects;
	}

	if (obj->mm.pages->nents != 1) {
		pr_err("%s object spans multiple sg entries\n", __func__);
		err = -EINVAL;
		goto err_close_objects;
	}

	igt_object_release(obj);

	/*
	 * Try to fragment the address space, such that half of it is free, but
	 * the max contiguous block size is SZ_64K.
	 */

	target = SZ_64K;
	n_objects = div64_u64(total, target);

	while (n_objects--) {
		struct list_head *list;

		if (n_objects % 2)
			list = &holes;
		else
			list = &objects;

		obj = igt_object_create(mem, list, target,
					I915_BO_ALLOC_CONTIGUOUS);
		if (IS_ERR(obj)) {
			err = PTR_ERR(obj);
			goto err_close_objects;
		}
	}

	close_objects(mem, &holes);

	min = target;
	target = total >> 1;

	/* Make sure we can still allocate all the fragmented space */
	obj = igt_object_create(mem, &objects, target, 0);
	if (IS_ERR(obj)) {
		err = PTR_ERR(obj);
		goto err_close_objects;
	}

	igt_object_release(obj);

	/*
	 * Even though we have enough free space, we don't have a big enough
	 * contiguous block. Make sure that holds true.
	 */

	do {
		bool should_fail = target > min;

		obj = igt_object_create(mem, &objects, target,
					I915_BO_ALLOC_CONTIGUOUS);
		if (should_fail != IS_ERR(obj)) {
			pr_err("%s target allocation(%llx) mismatch\n",
			       __func__, target);
			err = -EINVAL;
			goto err_close_objects;
		}

		target >>= 1;
	} while (target >= mem->mm.chunk_size);

err_close_objects:
	list_splice_tail(&holes, &objects);
	close_objects(mem, &objects);
	return err;
}

static int igt_mock_splintered_region(void *arg)
{
	struct intel_memory_region *mem = arg;
	struct drm_i915_private *i915 = mem->i915;
	struct drm_i915_gem_object *obj;
	unsigned int expected_order;
	LIST_HEAD(objects);
	u64 size;
	int err = 0;

	/*
	 * Sanity check we can still allocate everything even if the
	 * mm.max_order != mm.size. i.e our starting address space size is not a
	 * power-of-two.
	 */

	size = (SZ_4G - 1) & LINUX_PAGE_MASK;
	mem = mock_region_create(i915, 0, size, PAGE_SIZE, 0);
	if (IS_ERR(mem))
		return PTR_ERR(mem);

	if (mem->mm.size != size) {
		pr_err("%s size mismatch(%llu != %llu)\n",
		       __func__, mem->mm.size, size);
		err = -EINVAL;
		goto out_put;
	}

	expected_order = get_order(rounddown_pow_of_two(size));
	if (mem->mm.max_order != expected_order) {
		pr_err("%s order mismatch(%u != %u)\n",
		       __func__, mem->mm.max_order, expected_order);
		err = -EINVAL;
		goto out_put;
	}

	obj = igt_object_create(mem, &objects, size, 0);
	if (IS_ERR(obj)) {
		err = PTR_ERR(obj);
		goto out_close;
	}

	close_objects(mem, &objects);

	/*
	 * While we should be able allocate everything without any flag
	 * restrictions, if we consider I915_BO_ALLOC_CONTIGUOUS then we are
	 * actually limited to the largest power-of-two for the region size i.e
	 * max_order, due to the inner workings of the buddy allocator. So make
	 * sure that does indeed hold true.
	 */

	obj = igt_object_create(mem, &objects, size, I915_BO_ALLOC_CONTIGUOUS);
	if (!IS_ERR(obj)) {
		pr_err("%s too large contiguous allocation was not rejected\n",
		       __func__);
		err = -EINVAL;
		goto out_close;
	}

	obj = igt_object_create(mem, &objects, rounddown_pow_of_two(size),
				I915_BO_ALLOC_CONTIGUOUS);
	if (IS_ERR(obj)) {
		pr_err("%s largest possible contiguous allocation failed\n",
		       __func__);
		err = PTR_ERR(obj);
		goto out_close;
	}

out_close:
	close_objects(mem, &objects);
out_put:
	intel_memory_region_put(mem);
	return err;
}

static int igt_gpu_write_dw(struct intel_context *ce,
			    struct i915_vma *vma,
			    u32 dword,
			    u32 value)
{
	return igt_gpu_fill_dw(ce, vma, dword * sizeof(u32),
			       vma->size >> PAGE_SHIFT, value);
}

static int igt_cpu_check(struct drm_i915_gem_object *obj, u32 dword, u32 val)
{
	unsigned long n = obj->base.size >> PAGE_SHIFT;
	u32 *ptr;
	int err;

	err = i915_gem_object_wait(obj, 0, MAX_SCHEDULE_TIMEOUT);
	if (err)
		return err;

	ptr = i915_gem_object_pin_map(obj, I915_MAP_WC);
	if (IS_ERR(ptr))
		return PTR_ERR(ptr);

	ptr += dword;
	while (n--) {
		if (*ptr != val) {
			pr_err("base[%u]=%08x, val=%08x\n",
			       dword, *ptr, val);
			err = -EINVAL;
			break;
		}

		ptr += PAGE_SIZE / sizeof(*ptr);
	}

	i915_gem_object_unpin_map(obj);
	return err;
}

static int igt_gpu_write(struct i915_gem_context *ctx,
			 struct drm_i915_gem_object *obj)
{
	struct i915_gem_engines *engines;
	struct i915_gem_engines_iter it;
	struct i915_address_space *vm;
	struct intel_context *ce;
	I915_RND_STATE(prng);
	IGT_TIMEOUT(end_time);
	unsigned int count;
	struct i915_vma *vma;
	int *order;
	int i, n;
	int err = 0;

	GEM_BUG_ON(!i915_gem_object_has_pinned_pages(obj));

	n = 0;
	count = 0;
	for_each_gem_engine(ce, i915_gem_context_lock_engines(ctx), it) {
		count++;
		if (!intel_engine_can_store_dword(ce->engine))
			continue;

		vm = ce->vm;
		n++;
	}
	i915_gem_context_unlock_engines(ctx);
	if (!n)
		return 0;

	order = i915_random_order(count * count, &prng);
	if (!order)
		return -ENOMEM;

	vma = i915_vma_instance(obj, vm, NULL);
	if (IS_ERR(vma)) {
		err = PTR_ERR(vma);
		goto out_free;
	}

	err = i915_vma_pin(vma, 0, 0, PIN_USER);
	if (err)
		goto out_free;

	i = 0;
	engines = i915_gem_context_lock_engines(ctx);
	do {
		u32 rng = prandom_u32_state(&prng);
		u32 dword = offset_in_page(rng) / 4;

		ce = engines->engines[order[i] % engines->num_engines];
		i = (i + 1) % (count * count);
		if (!ce || !intel_engine_can_store_dword(ce->engine))
			continue;

		err = igt_gpu_write_dw(ce, vma, dword, rng);
		if (err)
			break;

		err = igt_cpu_check(obj, dword, rng);
		if (err)
			break;
	} while (!__igt_timeout(end_time, NULL));
	i915_gem_context_unlock_engines(ctx);

out_free:
	kfree(order);

	if (err == -ENOMEM)
		err = 0;

	return err;
}

static int igt_lmem_create(void *arg)
{
	struct drm_i915_private *i915 = arg;
	struct drm_i915_gem_object *obj;
	int err = 0;

	obj = i915_gem_object_create_lmem(i915, PAGE_SIZE, 0);
	if (IS_ERR(obj))
		return PTR_ERR(obj);

	err = i915_gem_object_pin_pages(obj);
	if (err)
		goto out_put;

	i915_gem_object_unpin_pages(obj);
out_put:
	i915_gem_object_put(obj);

	return err;
}

static int igt_lmem_write_gpu(void *arg)
{
	struct drm_i915_private *i915 = arg;
	struct drm_i915_gem_object *obj;
	struct i915_gem_context *ctx;
	struct file *file;
	I915_RND_STATE(prng);
	u32 sz;
	int err;

	file = mock_file(i915);
	if (IS_ERR(file))
		return PTR_ERR(file);

	ctx = live_context(i915, file);
	if (IS_ERR(ctx)) {
		err = PTR_ERR(ctx);
		goto out_file;
	}

	sz = round_up(prandom_u32_state(&prng) % SZ_32M, PAGE_SIZE);

	obj = i915_gem_object_create_lmem(i915, sz, 0);
	if (IS_ERR(obj)) {
		err = PTR_ERR(obj);
		goto out_file;
	}

	err = i915_gem_object_pin_pages(obj);
	if (err)
		goto out_put;

	err = igt_gpu_write(ctx, obj);
	if (err)
		pr_err("igt_gpu_write failed(%d)\n", err);

	i915_gem_object_unpin_pages(obj);
out_put:
	i915_gem_object_put(obj);
out_file:
	fput(file);
	return err;
}

static struct intel_engine_cs *
random_engine_class(struct drm_i915_private *i915,
		    unsigned int class,
		    struct rnd_state *prng)
{
	struct intel_engine_cs *engine;
	unsigned int count;

	count = 0;
	for (engine = intel_engine_lookup_user(i915, class, 0);
	     engine && engine->uabi_class == class;
	     engine = rb_entry_safe(rb_next(&engine->uabi_node),
				    typeof(*engine), uabi_node))
		count++;

	count = i915_prandom_u32_max_state(count, prng);
	return intel_engine_lookup_user(i915, class, count);
}

static int igt_lmem_write_cpu(void *arg)
{
	struct drm_i915_private *i915 = arg;
	struct drm_i915_gem_object *obj;
	I915_RND_STATE(prng);
	IGT_TIMEOUT(end_time);
	u32 bytes[] = {
		0, /* rng placeholder */
		sizeof(u32),
		sizeof(u64),
		64, /* cl */
		PAGE_SIZE,
		PAGE_SIZE - sizeof(u32),
		PAGE_SIZE - sizeof(u64),
		PAGE_SIZE - 64,
	};
	struct intel_engine_cs *engine;
	u32 *vaddr;
	u32 sz;
	u32 i;
	int *order;
	int count;
	int err;

	engine = random_engine_class(i915, I915_ENGINE_CLASS_COPY, &prng);
	if (!engine)
		return 0;

	pr_info("%s: using %s\n", __func__, engine->name);

	sz = round_up(prandom_u32_state(&prng) % SZ_32M, PAGE_SIZE);
	sz = max_t(u32, 2 * PAGE_SIZE, sz);

	obj = i915_gem_object_create_lmem(i915, sz, I915_BO_ALLOC_CONTIGUOUS);
	if (IS_ERR(obj))
		return PTR_ERR(obj);

	vaddr = i915_gem_object_pin_map(obj, I915_MAP_WC);
	if (IS_ERR(vaddr)) {
		err = PTR_ERR(vaddr);
		goto out_put;
	}

	/* Put the pages into a known state -- from the gpu for added fun */
	intel_engine_pm_get(engine);
	err = i915_gem_object_fill_blt(obj, engine->kernel_context, 0xdeadbeaf);
	intel_engine_pm_put(engine);
	if (err)
		goto out_unpin;

	i915_gem_object_lock(obj, NULL);
	err = i915_gem_object_set_to_wc_domain(obj, true);
	i915_gem_object_unlock(obj);
	if (err)
		goto out_unpin;

	count = ARRAY_SIZE(bytes);
	order = i915_random_order(count * count, &prng);
	if (!order) {
		err = -ENOMEM;
		goto out_unpin;
	}

	/* A random multiple of u32, picked between [64, PAGE_SIZE - 64] */
	bytes[0] = igt_random_offset(&prng, 64, PAGE_SIZE - 64, 0, sizeof(u32));
	GEM_BUG_ON(!IS_ALIGNED(bytes[0], sizeof(u32)));

	i = 0;
	do {
		u32 offset;
		u32 align;
		u32 dword;
		u32 size;
		u32 val;

		size = bytes[order[i] % count];
		i = (i + 1) % (count * count);

		align = bytes[order[i] % count];
		i = (i + 1) % (count * count);

		align = max_t(u32, sizeof(u32), rounddown_pow_of_two(align));

		offset = igt_random_offset(&prng, 0, obj->base.size,
					   size, align);

		val = prandom_u32_state(&prng);
		memset32(vaddr + offset / sizeof(u32), val ^ 0xdeadbeaf,
			 size / sizeof(u32));

		/*
		 * Sample random dw -- don't waste precious time reading every
		 * single dw.
		 */
		dword = igt_random_offset(&prng, offset,
					  offset + size,
					  sizeof(u32), sizeof(u32));
		dword /= sizeof(u32);
		if (vaddr[dword] != (val ^ 0xdeadbeaf)) {
			pr_err("%s vaddr[%u]=%u, val=%u, size=%u, align=%u, offset=%u\n",
			       __func__, dword, vaddr[dword], val ^ 0xdeadbeaf,
			       size, align, offset);
			err = -EINVAL;
			break;
		}
	} while (!__igt_timeout(end_time, NULL));

out_unpin:
	i915_gem_object_unpin_map(obj);
out_put:
	i915_gem_object_put(obj);

	return err;
}

static const char *repr_type(u32 type)
{
	switch (type) {
	case I915_MAP_WB:
		return "WB";
	case I915_MAP_WC:
		return "WC";
	}

	return "";
}

static struct drm_i915_gem_object *
create_region_for_mapping(struct intel_memory_region *mr, u64 size, u32 type,
			  void **out_addr)
{
	struct drm_i915_gem_object *obj;
	void *addr;

	obj = i915_gem_object_create_region(mr, size, 0);
	if (IS_ERR(obj)) {
		if (PTR_ERR(obj) == -ENOSPC) /* Stolen memory */
			return ERR_PTR(-ENODEV);
		return obj;
	}

	addr = i915_gem_object_pin_map(obj, type);
	if (IS_ERR(addr)) {
		i915_gem_object_put(obj);
		if (PTR_ERR(addr) == -ENXIO)
			return ERR_PTR(-ENODEV);
		return addr;
	}

	*out_addr = addr;
	return obj;
}

static int wrap_ktime_compare(const void *A, const void *B)
{
	const ktime_t *a = A, *b = B;

	return ktime_compare(*a, *b);
}

static void igt_memcpy_long(void *dst, const void *src, size_t size)
{
	unsigned long *tmp = dst;
	const unsigned long *s = src;

	size = size / sizeof(unsigned long);
	while (size--)
		*tmp++ = *s++;
}

static inline void igt_memcpy(void *dst, const void *src, size_t size)
{
	memcpy(dst, src, size);
}

static inline void igt_memcpy_from_wc(void *dst, const void *src, size_t size)
{
	i915_memcpy_from_wc(dst, src, size);
}

static int _perf_memcpy(struct intel_memory_region *src_mr,
			struct intel_memory_region *dst_mr,
			u64 size, u32 src_type, u32 dst_type)
{
	struct drm_i915_private *i915 = src_mr->i915;
	const struct {
		const char *name;
		void (*copy)(void *dst, const void *src, size_t size);
		bool skip;
	} tests[] = {
		{
			"memcpy",
			igt_memcpy,
		},
		{
			"memcpy_long",
			igt_memcpy_long,
		},
		{
			"memcpy_from_wc",
			igt_memcpy_from_wc,
			!i915_has_memcpy_from_wc(),
		},
	};
	struct drm_i915_gem_object *src, *dst;
	void *src_addr, *dst_addr;
	int ret = 0;
	int i;

	src = create_region_for_mapping(src_mr, size, src_type, &src_addr);
	if (IS_ERR(src)) {
		ret = PTR_ERR(src);
		goto out;
	}

	dst = create_region_for_mapping(dst_mr, size, dst_type, &dst_addr);
	if (IS_ERR(dst)) {
		ret = PTR_ERR(dst);
		goto out_unpin_src;
	}

	for (i = 0; i < ARRAY_SIZE(tests); ++i) {
		ktime_t t[5];
		int pass;

		if (tests[i].skip)
			continue;

		for (pass = 0; pass < ARRAY_SIZE(t); pass++) {
			ktime_t t0, t1;

			t0 = ktime_get();

			tests[i].copy(dst_addr, src_addr, size);

			t1 = ktime_get();
			t[pass] = ktime_sub(t1, t0);
		}

		sort(t, ARRAY_SIZE(t), sizeof(*t), wrap_ktime_compare, NULL);
		pr_info("%s src(%s, %s) -> dst(%s, %s) %14s %4llu KiB copy: %5lld MiB/s\n",
			__func__,
			src_mr->name,
			repr_type(src_type),
			dst_mr->name,
			repr_type(dst_type),
			tests[i].name,
			size >> 10,
			div64_u64(mul_u32_u32(4 * size,
					      1000 * 1000 * 1000),
				  t[1] + 2 * t[2] + t[3]) >> 20);

		cond_resched();
	}

	i915_gem_object_unpin_map(dst);
	i915_gem_object_put(dst);
out_unpin_src:
	i915_gem_object_unpin_map(src);
	i915_gem_object_put(src);

	i915_gem_drain_freed_objects(i915);
out:
	if (ret == -ENODEV)
		ret = 0;

	return ret;
}

static int perf_memcpy(void *arg)
{
	struct drm_i915_private *i915 = arg;
	static const u32 types[] = {
		I915_MAP_WB,
		I915_MAP_WC,
	};
	static const u32 sizes[] = {
		SZ_4K,
		SZ_64K,
		SZ_4M,
	};
	struct intel_memory_region *src_mr, *dst_mr;
	int src_id, dst_id;
	int i, j, k;
	int ret;

	for_each_memory_region(src_mr, i915, src_id) {
		for_each_memory_region(dst_mr, i915, dst_id) {
			for (i = 0; i < ARRAY_SIZE(sizes); ++i) {
				for (j = 0; j < ARRAY_SIZE(types); ++j) {
					for (k = 0; k < ARRAY_SIZE(types); ++k) {
						ret = _perf_memcpy(src_mr,
								   dst_mr,
								   sizes[i],
								   types[j],
								   types[k]);
						if (ret)
							return ret;
					}
				}
			}
		}
	}

	return 0;
}

int intel_memory_region_mock_selftests(void)
{
	static const struct i915_subtest tests[] = {
		SUBTEST(igt_mock_fill),
		SUBTEST(igt_mock_contiguous),
		SUBTEST(igt_mock_splintered_region),
	};
	struct intel_memory_region *mem;
	struct drm_i915_private *i915;
	int err;

	i915 = mock_gem_device();
	if (!i915)
		return -ENOMEM;

	mem = mock_region_create(i915, 0, SZ_2G, I915_GTT_PAGE_SIZE_4K, 0);
	if (IS_ERR(mem)) {
		pr_err("failed to create memory region\n");
		err = PTR_ERR(mem);
		goto out_unref;
	}

	err = i915_subtests(tests, mem);

	intel_memory_region_put(mem);
out_unref:
	mock_destroy_device(i915);
	return err;
}

int intel_memory_region_live_selftests(struct drm_i915_private *i915)
{
	static const struct i915_subtest tests[] = {
		SUBTEST(igt_lmem_create),
		SUBTEST(igt_lmem_write_cpu),
		SUBTEST(igt_lmem_write_gpu),
	};

	if (!HAS_LMEM(i915)) {
		pr_info("device lacks LMEM support, skipping\n");
		return 0;
	}

	if (intel_gt_is_wedged(&i915->gt))
		return 0;

	return i915_live_subtests(tests, i915);
}

int intel_memory_region_perf_selftests(struct drm_i915_private *i915)
{
	static const struct i915_subtest tests[] = {
		SUBTEST(perf_memcpy),
	};

	if (intel_gt_is_wedged(&i915->gt))
		return 0;

	return i915_live_subtests(tests, i915);
}
