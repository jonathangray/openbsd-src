/**************************************************************************
 *
 * Copyright 2006 Tungsten Graphics, Inc., Bismarck, ND., USA.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 **************************************************************************/

/*
 * Generic simple memory manager implementation. Intended to be used as a base
 * class implementation for more advanced memory managers.
 *
 * Note that the algorithm used is quite simple and there might be substantial
 * performance gains if a smarter free list is implemented. Currently it is just an
 * unordered stack of free regions. This could easily be improved if an RB-tree
 * is used instead. At least if we expect heavy fragmentation.
 *
 * Aligned allocations can also see improvement.
 *
 * Authors:
 * Thomas Hellström <thomas-at-tungstengraphics-dot-com>
 */

#include <drm/drmP.h>
#include <drm/drm_mm.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/export.h>

/**
 * DOC: Overview
 *
 * drm_mm provides a simple range allocator. The drivers are free to use the
 * resource allocator from the linux core if it suits them, the upside of drm_mm
 * is that it's in the DRM core. Which means that it's easier to extend for
 * some of the crazier special purpose needs of gpus.
 *
 * The main data struct is &drm_mm, allocations are tracked in &drm_mm_node.
 * Drivers are free to embed either of them into their own suitable
 * datastructures. drm_mm itself will not do any allocations of its own, so if
 * drivers choose not to embed nodes they need to still allocate them
 * themselves.
 *
 * The range allocator also supports reservation of preallocated blocks. This is
 * useful for taking over initial mode setting configurations from the firmware,
 * where an object needs to be created which exactly matches the firmware's
 * scanout target. As long as the range is still free it can be inserted anytime
 * after the allocator is initialized, which helps with avoiding looped
 * depencies in the driver load sequence.
 *
 * drm_mm maintains a stack of most recently freed holes, which of all
 * simplistic datastructures seems to be a fairly decent approach to clustering
 * allocations and avoiding too much fragmentation. This means free space
 * searches are O(num_holes). Given that all the fancy features drm_mm supports
 * something better would be fairly complex and since gfx thrashing is a fairly
 * steep cliff not a real concern. Removing a node again is O(1).
 *
 * drm_mm supports a few features: Alignment and range restrictions can be
 * supplied. Further more every &drm_mm_node has a color value (which is just an
 * opaqua unsigned long) which in conjunction with a driver callback can be used
 * to implement sophisticated placement restrictions. The i915 DRM driver uses
 * this to implement guard pages between incompatible caching domains in the
 * graphics TT.
 *
 * Two behaviors are supported for searching and allocating: bottom-up and top-down.
 * The default is bottom-up. Top-down allocation can be used if the memory area
 * has different restrictions, or just to reduce fragmentation.
 *
 * Finally iteration helpers to walk all nodes and all holes are provided as are
 * some basic allocator dumpers for debugging.
 */

struct drm_mm_node *
__drm_mm_interval_first(const struct drm_mm *mm, u64 start, u64 last)
{
	struct drm_mm_node *node;

	drm_mm_for_each_node(node, mm) {
		if (node->start + node->size > start &&
		    node->start < last)
			return node;
	}

	return &mm->head_node;
}

static struct drm_mm_node *drm_mm_search_free_generic(const struct drm_mm *mm,
						u64 size,
						unsigned alignment,
						unsigned long color,
						enum drm_mm_search_flags flags);
static struct drm_mm_node *drm_mm_search_free_in_range_generic(const struct drm_mm *mm,
						u64 size,
						unsigned alignment,
						unsigned long color,
						u64 start,
						u64 end,
						enum drm_mm_search_flags flags);

static void drm_mm_insert_helper(struct drm_mm_node *hole_node,
				 struct drm_mm_node *node,
				 u64 size, unsigned alignment,
				 unsigned long color,
				 enum drm_mm_allocator_flags flags)
{
	struct drm_mm *mm = hole_node->mm;
	u64 hole_start = drm_mm_hole_node_start(hole_node);
	u64 hole_end = drm_mm_hole_node_end(hole_node);
	u64 adj_start = hole_start;
	u64 adj_end = hole_end;

	BUG_ON(node->allocated);

	if (mm->color_adjust)
		mm->color_adjust(hole_node, color, &adj_start, &adj_end);

	if (flags & DRM_MM_CREATE_TOP)
		adj_start = adj_end - size;

	if (alignment) {
		u64 tmp = adj_start;
		unsigned rem;

		rem = do_div(tmp, alignment);
		if (rem) {
			if (flags & DRM_MM_CREATE_TOP)
				adj_start -= rem;
			else
				adj_start += alignment - rem;
		}
	}

	BUG_ON(adj_start < hole_start);
	BUG_ON(adj_end > hole_end);

	if (adj_start == hole_start) {
		hole_node->hole_follows = 0;
		list_del(&hole_node->hole_stack);
	}

	node->start = adj_start;
	node->size = size;
	node->mm = mm;
	node->color = color;
	node->allocated = 1;

	INIT_LIST_HEAD(&node->hole_stack);
	list_add(&node->node_list, &hole_node->node_list);

	BUG_ON(node->start + node->size > adj_end);

	node->hole_follows = 0;
	if (__drm_mm_hole_node_start(node) < hole_end) {
		list_add(&node->hole_stack, &mm->hole_stack);
		node->hole_follows = 1;
	}
}

/**
 * drm_mm_reserve_node - insert an pre-initialized node
 * @mm: drm_mm allocator to insert @node into
 * @node: drm_mm_node to insert
 *
 * This functions inserts an already set-up drm_mm_node into the allocator,
 * meaning that start, size and color must be set by the caller. This is useful
 * to initialize the allocator with preallocated objects which must be set-up
 * before the range allocator can be set-up, e.g. when taking over a firmware
 * framebuffer.
 *
 * Returns:
 * 0 on success, -ENOSPC if there's no hole where @node is.
 */
int drm_mm_reserve_node(struct drm_mm *mm, struct drm_mm_node *node)
{
	struct drm_mm_node *hole;
	u64 end;
	u64 hole_start;
	u64 hole_end;

	BUG_ON(node == NULL);

	end = node->start + node->size;

	/* Find the relevant hole to add our node to */
	drm_mm_for_each_hole(hole, mm, hole_start, hole_end) {
		if (hole_start > node->start || hole_end < end)
			continue;

		node->mm = mm;
		node->allocated = 1;

		INIT_LIST_HEAD(&node->hole_stack);
		list_add(&node->node_list, &hole->node_list);

		if (node->start == hole_start) {
			hole->hole_follows = 0;
			list_del_init(&hole->hole_stack);
		}

		node->hole_follows = 0;
		if (end != hole_end) {
			list_add(&node->hole_stack, &mm->hole_stack);
			node->hole_follows = 1;
		}

		return 0;
	}

	return -ENOSPC;
}
EXPORT_SYMBOL(drm_mm_reserve_node);

/**
 * drm_mm_insert_node_generic - search for space and insert @node
 * @mm: drm_mm to allocate from
 * @node: preallocate node to insert
 * @size: size of the allocation
 * @alignment: alignment of the allocation
 * @color: opaque tag value to use for this node
 * @sflags: flags to fine-tune the allocation search
 * @aflags: flags to fine-tune the allocation behavior
 *
 * The preallocated node must be cleared to 0.
 *
 * Returns:
 * 0 on success, -ENOSPC if there's no suitable hole.
 */
int drm_mm_insert_node_generic(struct drm_mm *mm, struct drm_mm_node *node,
			       u64 size, unsigned alignment,
			       unsigned long color,
			       enum drm_mm_search_flags sflags,
			       enum drm_mm_allocator_flags aflags)
{
	struct drm_mm_node *hole_node;

	hole_node = drm_mm_search_free_generic(mm, size, alignment,
					       color, sflags);
	if (!hole_node)
		return -ENOSPC;

	drm_mm_insert_helper(hole_node, node, size, alignment, color, aflags);
	return 0;
}
EXPORT_SYMBOL(drm_mm_insert_node_generic);

static void drm_mm_insert_helper_range(struct drm_mm_node *hole_node,
				       struct drm_mm_node *node,
				       u64 size, unsigned alignment,
				       unsigned long color,
				       u64 start, u64 end,
				       enum drm_mm_allocator_flags flags)
{
	struct drm_mm *mm = hole_node->mm;
	u64 hole_start = drm_mm_hole_node_start(hole_node);
	u64 hole_end = drm_mm_hole_node_end(hole_node);
	u64 adj_start = hole_start;
	u64 adj_end = hole_end;

	BUG_ON(!hole_node->hole_follows || node->allocated);

	if (adj_start < start)
		adj_start = start;
	if (adj_end > end)
		adj_end = end;

	if (mm->color_adjust)
		mm->color_adjust(hole_node, color, &adj_start, &adj_end);

	if (flags & DRM_MM_CREATE_TOP)
		adj_start = adj_end - size;

	if (alignment) {
		u64 tmp = adj_start;
		unsigned rem;

		rem = do_div(tmp, alignment);
		if (rem) {
			if (flags & DRM_MM_CREATE_TOP)
				adj_start -= rem;
			else
				adj_start += alignment - rem;
		}
	}

	if (adj_start == hole_start) {
		hole_node->hole_follows = 0;
		list_del(&hole_node->hole_stack);
	}

	node->start = adj_start;
	node->size = size;
	node->mm = mm;
	node->color = color;
	node->allocated = 1;

	INIT_LIST_HEAD(&node->hole_stack);
	list_add(&node->node_list, &hole_node->node_list);

	BUG_ON(node->start < start);
	BUG_ON(node->start < adj_start);
	BUG_ON(node->start + node->size > adj_end);
	BUG_ON(node->start + node->size > end);

	node->hole_follows = 0;
	if (__drm_mm_hole_node_start(node) < hole_end) {
		list_add(&node->hole_stack, &mm->hole_stack);
		node->hole_follows = 1;
	}
}

/**
 * drm_mm_insert_node_in_range_generic - ranged search for space and insert @node
 * @mm: drm_mm to allocate from
 * @node: preallocate node to insert
 * @size: size of the allocation
 * @alignment: alignment of the allocation
 * @color: opaque tag value to use for this node
 * @start: start of the allowed range for this node
 * @end: end of the allowed range for this node
 * @sflags: flags to fine-tune the allocation search
 * @aflags: flags to fine-tune the allocation behavior
 *
 * The preallocated node must be cleared to 0.
 *
 * Returns:
 * 0 on success, -ENOSPC if there's no suitable hole.
 */
int drm_mm_insert_node_in_range_generic(struct drm_mm *mm, struct drm_mm_node *node,
					u64 size, unsigned alignment,
					unsigned long color,
					u64 start, u64 end,
					enum drm_mm_search_flags sflags,
					enum drm_mm_allocator_flags aflags)
{
	struct drm_mm_node *hole_node;

	hole_node = drm_mm_search_free_in_range_generic(mm,
							size, alignment, color,
							start, end, sflags);
	if (!hole_node)
		return -ENOSPC;

	drm_mm_insert_helper_range(hole_node, node,
				   size, alignment, color,
				   start, end, aflags);
	return 0;
}
EXPORT_SYMBOL(drm_mm_insert_node_in_range_generic);

/**
 * drm_mm_remove_node - Remove a memory node from the allocator.
 * @node: drm_mm_node to remove
 *
 * This just removes a node from its drm_mm allocator. The node does not need to
 * be cleared again before it can be re-inserted into this or any other drm_mm
 * allocator. It is a bug to call this function on a un-allocated node.
 */
void drm_mm_remove_node(struct drm_mm_node *node)
{
	struct drm_mm *mm = node->mm;
	struct drm_mm_node *prev_node;

	if (WARN_ON(!node->allocated))
		return;

	BUG_ON(node->scanned_block || node->scanned_prev_free
				   || node->scanned_next_free);

	prev_node =
	    list_entry(node->node_list.prev, struct drm_mm_node, node_list);

	if (node->hole_follows) {
		BUG_ON(__drm_mm_hole_node_start(node) ==
		       __drm_mm_hole_node_end(node));
		list_del(&node->hole_stack);
	} else
		BUG_ON(__drm_mm_hole_node_start(node) !=
		       __drm_mm_hole_node_end(node));


	if (!prev_node->hole_follows) {
		prev_node->hole_follows = 1;
		list_add(&prev_node->hole_stack, &mm->hole_stack);
	} else
		list_move(&prev_node->hole_stack, &mm->hole_stack);

	list_del(&node->node_list);
	node->allocated = 0;
}
EXPORT_SYMBOL(drm_mm_remove_node);

static int check_free_hole(u64 start, u64 end, u64 size, unsigned alignment)
{
	if (end - start < size)
		return 0;

	if (alignment) {
		u64 tmp = start;
		unsigned rem;

		rem = do_div(tmp, alignment);
		if (rem)
			start += alignment - rem;
	}

	return end >= start + size;
}

static struct drm_mm_node *drm_mm_search_free_generic(const struct drm_mm *mm,
						      u64 size,
						      unsigned alignment,
						      unsigned long color,
						      enum drm_mm_search_flags flags)
{
	struct drm_mm_node *entry;
	struct drm_mm_node *best;
	u64 adj_start;
	u64 adj_end;
	u64 best_size;

	BUG_ON(mm->scan_active);

	best = NULL;
	best_size = ~0UL;

	__drm_mm_for_each_hole(entry, mm, adj_start, adj_end,
			       flags & DRM_MM_SEARCH_BELOW) {
		u64 hole_size = adj_end - adj_start;

		if (mm->color_adjust) {
			mm->color_adjust(entry, color, &adj_start, &adj_end);
			if (adj_end <= adj_start)
				continue;
		}

		if (!check_free_hole(adj_start, adj_end, size, alignment))
			continue;

		if (!(flags & DRM_MM_SEARCH_BEST))
			return entry;

		if (hole_size < best_size) {
			best = entry;
			best_size = hole_size;
		}
	}

	return best;
}

static struct drm_mm_node *drm_mm_search_free_in_range_generic(const struct drm_mm *mm,
							u64 size,
							unsigned alignment,
							unsigned long color,
							u64 start,
							u64 end,
							enum drm_mm_search_flags flags)
{
	struct drm_mm_node *entry;
	struct drm_mm_node *best;
	u64 adj_start;
	u64 adj_end;
	u64 best_size;

	BUG_ON(mm->scan_active);

	best = NULL;
	best_size = ~0UL;

	__drm_mm_for_each_hole(entry, mm, adj_start, adj_end,
			       flags & DRM_MM_SEARCH_BELOW) {
		u64 hole_size = adj_end - adj_start;

		if (adj_start < start)
			adj_start = start;
		if (adj_end > end)
			adj_end = end;

		if (mm->color_adjust) {
			mm->color_adjust(entry, color, &adj_start, &adj_end);
			if (adj_end <= adj_start)
				continue;
		}

		if (!check_free_hole(adj_start, adj_end, size, alignment))
			continue;

		if (!(flags & DRM_MM_SEARCH_BEST))
			return entry;

		if (hole_size < best_size) {
			best = entry;
			best_size = hole_size;
		}
	}

	return best;
}

/**
 * drm_mm_replace_node - move an allocation from @old to @new
 * @old: drm_mm_node to remove from the allocator
 * @new: drm_mm_node which should inherit @old's allocation
 *
 * This is useful for when drivers embed the drm_mm_node structure and hence
 * can't move allocations by reassigning pointers. It's a combination of remove
 * and insert with the guarantee that the allocation start will match.
 */
void drm_mm_replace_node(struct drm_mm_node *old, struct drm_mm_node *new)
{
	list_replace(&old->node_list, &new->node_list);
	list_replace(&old->hole_stack, &new->hole_stack);
	new->hole_follows = old->hole_follows;
	new->mm = old->mm;
	new->start = old->start;
	new->size = old->size;
	new->color = old->color;

	old->allocated = 0;
	new->allocated = 1;
}
EXPORT_SYMBOL(drm_mm_replace_node);

/**
 * DOC: lru scan roster
 *
 * Very often GPUs need to have continuous allocations for a given object. When
 * evicting objects to make space for a new one it is therefore not most
 * efficient when we simply start to select all objects from the tail of an LRU
 * until there's a suitable hole: Especially for big objects or nodes that
 * otherwise have special allocation constraints there's a good chance we evict
 * lots of (smaller) objects unnecessarily.
 *
 * The DRM range allocator supports this use-case through the scanning
 * interfaces. First a scan operation needs to be initialized with
 * drm_mm_scan_init() or drm_mm_scan_init_with_range(). The driver adds
 * objects to the roster, probably by walking an LRU list, but this can be
 * freely implemented. Eviction candiates are added using
 * drm_mm_scan_add_block() until a suitable hole is found or there are no
 * further evictable objects. Eviction roster metadata is tracked in &struct
 * drm_mm_scan.
 *
 * The driver must walk through all objects again in exactly the reverse
 * order to restore the allocator state. Note that while the allocator is used
 * in the scan mode no other operation is allowed.
 *
 * Finally the driver evicts all objects selected (drm_mm_scan_remove_block()
 * reported true) in the scan, and any overlapping nodes after color adjustment
 * (drm_mm_scan_color_evict()). Adding and removing an object is O(1), and
 * since freeing a node is also O(1) the overall complexity is
 * O(scanned_objects). So like the free stack which needs to be walked before a
 * scan operation even begins this is linear in the number of objects. It
 * doesn't seem to hurt too badly.
 */

/**
 * drm_mm_scan_init_with_range - initialize range-restricted lru scanning
 * @scan: scan state
 * @mm: drm_mm to scan
 * @size: size of the allocation
 * @alignment: alignment of the allocation
 * @color: opaque tag value to use for the allocation
 * @start: start of the allowed range for the allocation
 * @end: end of the allowed range for the allocation
 * @mode: fine-tune the allocation search and placement
 *
 * This simply sets up the scanning routines with the parameters for the desired
 * hole.
 *
 * Warning:
 * As long as the scan list is non-empty, no other operations than
 * adding/removing nodes to/from the scan list are allowed.
 */
void drm_mm_scan_init_with_range(struct drm_mm_scan *scan,
				 struct drm_mm *mm,
				 u64 size,
				 u64 alignment,
				 unsigned long color,
				 u64 start,
				 u64 end,
				 enum drm_mm_insert_mode mode)
{
	DRM_MM_BUG_ON(start >= end);
	DRM_MM_BUG_ON(!size || size > end - start);
	DRM_MM_BUG_ON(mm->scan_active);

	scan->mm = mm;

	if (alignment <= 1)
		alignment = 0;

	scan->color = color;
	scan->alignment = alignment;
	scan->remainder_mask = is_power_of_2(alignment) ? alignment - 1 : 0;
	scan->size = size;
	scan->mode = mode;

	DRM_MM_BUG_ON(end <= start);
	scan->range_start = start;
	scan->range_end = end;

	scan->hit_start = U64_MAX;
	scan->hit_end = 0;
}
EXPORT_SYMBOL(drm_mm_scan_init_with_range);

/**
 * drm_mm_scan_add_block - add a node to the scan list
 * @scan: the active drm_mm scanner
 * @node: drm_mm_node to add
 *
 * Add a node to the scan list that might be freed to make space for the desired
 * hole.
 *
 * Returns:
 * True if a hole has been found, false otherwise.
 */
bool drm_mm_scan_add_block(struct drm_mm_scan *scan,
			   struct drm_mm_node *node)
{
	struct drm_mm *mm = scan->mm;
	struct drm_mm_node *hole;
	u64 hole_start, hole_end;
	u64 col_start, col_end;
	u64 adj_start, adj_end;

	DRM_MM_BUG_ON(node->mm != mm);
	DRM_MM_BUG_ON(!node->allocated);
	DRM_MM_BUG_ON(node->scanned_block);
	node->scanned_block = true;
	mm->scan_active++;

	/* Remove this block from the node_list so that we enlarge the hole
	 * (distance between the end of our previous node and the start of
	 * or next), without poisoning the link so that we can restore it
	 * later in drm_mm_scan_remove_block().
	 */
	hole = list_prev_entry(node, node_list);
	DRM_MM_BUG_ON(list_next_entry(hole, node_list) != node);
	__list_del_entry(&node->node_list);

	hole_start = __drm_mm_hole_node_start(hole);
	hole_end = __drm_mm_hole_node_end(hole);

	col_start = hole_start;
	col_end = hole_end;
	if (mm->color_adjust)
		mm->color_adjust(hole, scan->color, &col_start, &col_end);

	adj_start = max(col_start, scan->range_start);
	adj_end = min(col_end, scan->range_end);
	if (adj_end <= adj_start || adj_end - adj_start < scan->size)
		return false;

	if (scan->mode == DRM_MM_INSERT_HIGH)
		adj_start = adj_end - scan->size;

	if (scan->alignment) {
		u64 rem;

		if (likely(scan->remainder_mask))
			rem = adj_start & scan->remainder_mask;
		else
			div64_u64_rem(adj_start, scan->alignment, &rem);
		if (rem) {
			adj_start -= rem;
			if (scan->mode != DRM_MM_INSERT_HIGH)
				adj_start += scan->alignment;
			if (adj_start < max(col_start, scan->range_start) ||
			    min(col_end, scan->range_end) - adj_start < scan->size)
				return false;

			if (adj_end <= adj_start ||
			    adj_end - adj_start < scan->size)
				return false;
		}
	}

	scan->hit_start = adj_start;
	scan->hit_end = adj_start + scan->size;

	DRM_MM_BUG_ON(scan->hit_start >= scan->hit_end);
	DRM_MM_BUG_ON(scan->hit_start < hole_start);
	DRM_MM_BUG_ON(scan->hit_end > hole_end);

	return true;
}
EXPORT_SYMBOL(drm_mm_scan_add_block);

/**
 * drm_mm_scan_remove_block - remove a node from the scan list
 * @scan: the active drm_mm scanner
 * @node: drm_mm_node to remove
 *
 * Nodes **must** be removed in exactly the reverse order from the scan list as
 * they have been added (e.g. using list_add() as they are added and then
 * list_for_each() over that eviction list to remove), otherwise the internal
 * state of the memory manager will be corrupted.
 *
 * When the scan list is empty, the selected memory nodes can be freed. An
 * immediately following drm_mm_insert_node_in_range_generic() or one of the
 * simpler versions of that function with !DRM_MM_SEARCH_BEST will then return
 * the just freed block (because its at the top of the free_stack list).
 *
 * Returns:
 * True if this block should be evicted, false otherwise. Will always
 * return false when no hole has been found.
 */
bool drm_mm_scan_remove_block(struct drm_mm_scan *scan,
			      struct drm_mm_node *node)
{
	struct drm_mm_node *prev_node;

	DRM_MM_BUG_ON(node->mm != scan->mm);
	DRM_MM_BUG_ON(!node->scanned_block);
	node->scanned_block = false;

	DRM_MM_BUG_ON(!node->mm->scan_active);
	node->mm->scan_active--;

	/* During drm_mm_scan_add_block() we decoupled this node leaving
	 * its pointers intact. Now that the caller is walking back along
	 * the eviction list we can restore this block into its rightful
	 * place on the full node_list. To confirm that the caller is walking
	 * backwards correctly we check that prev_node->next == node->next,
	 * i.e. both believe the same node should be on the other side of the
	 * hole.
	 */
	prev_node = list_prev_entry(node, node_list);
	DRM_MM_BUG_ON(list_next_entry(prev_node, node_list) !=
		      list_next_entry(node, node_list));
	list_add(&node->node_list, &prev_node->node_list);

	return (node->start + node->size > scan->hit_start &&
		node->start < scan->hit_end);
}
EXPORT_SYMBOL(drm_mm_scan_remove_block);

/**
 * drm_mm_scan_color_evict - evict overlapping nodes on either side of hole
 * @scan: drm_mm scan with target hole
 *
 * After completing an eviction scan and removing the selected nodes, we may
 * need to remove a few more nodes from either side of the target hole if
 * mm.color_adjust is being used.
 *
 * Returns:
 * A node to evict, or NULL if there are no overlapping nodes.
 */
struct drm_mm_node *drm_mm_scan_color_evict(struct drm_mm_scan *scan)
{
	STUB();
	return NULL;
#if 0
	struct drm_mm *mm = scan->mm;
	struct drm_mm_node *hole;
	u64 hole_start, hole_end;

	DRM_MM_BUG_ON(list_empty(&mm->hole_stack));

	if (!mm->color_adjust)
		return NULL;

	/*
	 * The hole found during scanning should ideally be the first element
	 * in the hole_stack list, but due to side-effects in the driver it
	 * may not be.
	 */
	list_for_each_entry(hole, &mm->hole_stack, hole_stack) {
		hole_start = __drm_mm_hole_node_start(hole);
		hole_end = hole_start + hole->hole_size;

		if (hole_start <= scan->hit_start &&
		    hole_end >= scan->hit_end)
			break;
	}

	/* We should only be called after we found the hole previously */
	DRM_MM_BUG_ON(&hole->hole_stack == &mm->hole_stack);
	if (unlikely(&hole->hole_stack == &mm->hole_stack))
		return NULL;

	DRM_MM_BUG_ON(hole_start > scan->hit_start);
	DRM_MM_BUG_ON(hole_end < scan->hit_end);

	mm->color_adjust(hole, scan->color, &hole_start, &hole_end);
	if (hole_start > scan->hit_start)
		return hole;
	if (hole_end < scan->hit_end)
		return list_next_entry(hole, node_list);

	return NULL;
#endif
}
EXPORT_SYMBOL(drm_mm_scan_color_evict);

/**
 * drm_mm_clean - checks whether an allocator is clean
 * @mm: drm_mm allocator to check
 *
 * Returns:
 * True if the allocator is completely free, false if there's still a node
 * allocated in it.
 */
bool drm_mm_clean(const struct drm_mm * mm)
{
	struct list_head *head = &mm->head_node.node_list;

	return (head->next->next == head);
}
EXPORT_SYMBOL(drm_mm_clean);

/**
 * drm_mm_init - initialize a drm-mm allocator
 * @mm: the drm_mm structure to initialize
 * @start: start of the range managed by @mm
 * @size: end of the range managed by @mm
 *
 * Note that @mm must be cleared to 0 before calling this function.
 */
void drm_mm_init(struct drm_mm *mm, u64 start, u64 size)
{
	INIT_LIST_HEAD(&mm->hole_stack);
	mm->scan_active = 0;

	/* Clever trick to avoid a special case in the free hole tracking. */
	INIT_LIST_HEAD(&mm->head_node.node_list);
	INIT_LIST_HEAD(&mm->head_node.hole_stack);
	mm->head_node.hole_follows = 1;
	mm->head_node.scanned_block = 0;
	mm->head_node.scanned_prev_free = 0;
	mm->head_node.scanned_next_free = 0;
	mm->head_node.mm = mm;
	mm->head_node.start = start + size;
	mm->head_node.size = start - mm->head_node.start;
	list_add_tail(&mm->head_node.hole_stack, &mm->hole_stack);

	mm->color_adjust = NULL;
}
EXPORT_SYMBOL(drm_mm_init);

/**
 * drm_mm_takedown - clean up a drm_mm allocator
 * @mm: drm_mm allocator to clean up
 *
 * Note that it is a bug to call this function on an allocator which is not
 * clean.
 */
void drm_mm_takedown(struct drm_mm * mm)
{
	WARN(!list_empty(&mm->head_node.node_list),
	     "Memory manager not clean during takedown.\n");
}
EXPORT_SYMBOL(drm_mm_takedown);

static u64 drm_mm_debug_hole(const struct drm_mm_node *entry,
			     const char *prefix)
{
	u64 hole_start, hole_end, hole_size;

	if (entry->hole_follows) {
		hole_start = drm_mm_hole_node_start(entry);
		hole_end = drm_mm_hole_node_end(entry);
		hole_size = hole_end - hole_start;
		pr_debug("%s %#llx-%#llx: %llu: free\n", prefix, hole_start,
			 hole_end, hole_size);
		return hole_size;
	}

	return 0;
}

/**
 * drm_mm_debug_table - dump allocator state to dmesg
 * @mm: drm_mm allocator to dump
 * @prefix: prefix to use for dumping to dmesg
 */
void drm_mm_debug_table(const struct drm_mm *mm, const char *prefix)
{
	struct drm_mm_node *entry;
	u64 total_used = 0, total_free = 0, total = 0;

	total_free += drm_mm_debug_hole(&mm->head_node, prefix);

	drm_mm_for_each_node(entry, mm) {
		pr_debug("%s %#llx-%#llx: %llu: used\n", prefix, entry->start,
			 entry->start + entry->size, entry->size);
		total_used += entry->size;
		total_free += drm_mm_debug_hole(entry, prefix);
	}
	total = total_free + total_used;

	pr_debug("%s total: %llu, used %llu free %llu\n", prefix, total,
		 total_used, total_free);
}
EXPORT_SYMBOL(drm_mm_debug_table);

static u64 drm_mm_dump_hole(struct drm_printer *p, const struct drm_mm_node *entry)
{
	u64 start, end, size;

	if (entry->hole_follows) {
		start = drm_mm_hole_node_start(entry);
		end = drm_mm_hole_node_end(entry);
		size = end - start;
		drm_printf(p, "%#018llx-%#018llx: %llu: free\n",
			   start, start + size, size);
		return size;
	}

	return 0;
}
/**
 * drm_mm_print - print allocator state
 * @mm: drm_mm allocator to print
 * @p: DRM printer to use
 */
void drm_mm_print(const struct drm_mm *mm, struct drm_printer *p)
{
	const struct drm_mm_node *entry;
	u64 total_used = 0, total_free = 0, total = 0;

	total_free += drm_mm_dump_hole(p, &mm->head_node);

	drm_mm_for_each_node(entry, mm) {
		drm_printf(p, "%#018llx-%#018llx: %llu: used\n", entry->start,
			   entry->start + entry->size, entry->size);
		total_used += entry->size;
		total_free += drm_mm_dump_hole(p, entry);
	}
	total = total_free + total_used;

	drm_printf(p, "total: %llu, used %llu free %llu\n", total,
		   total_used, total_free);
}
EXPORT_SYMBOL(drm_mm_print);
