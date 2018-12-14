/**************************************************************************
 *
 * Copyright 2006-2008 Tungsten Graphics, Inc., Cedar Park, TX. USA.
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
 * Authors:
 * Thomas Hellstrom <thomas-at-tungstengraphics-dot-com>
 */

#ifndef _DRM_MM_H_
#define _DRM_MM_H_

/*
 * Generic range manager structs
 */
#include <linux/bug.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#ifdef CONFIG_DEBUG_FS
#include <linux/seq_file.h>
#endif

#ifdef CONFIG_DRM_DEBUG_MM
#define DRM_MM_BUG_ON(expr) BUG_ON(expr)
#else
#define DRM_MM_BUG_ON(expr) BUILD_BUG_ON_INVALID(expr)
#endif

enum drm_mm_search_flags {
	DRM_MM_SEARCH_DEFAULT =		0,
	DRM_MM_SEARCH_BEST =		1 << 0,
	DRM_MM_SEARCH_BELOW =		1 << 1,
};

enum drm_mm_allocator_flags {
	DRM_MM_CREATE_DEFAULT =		0,
	DRM_MM_CREATE_TOP =		1 << 0,
};

#define DRM_MM_BOTTOMUP DRM_MM_SEARCH_DEFAULT, DRM_MM_CREATE_DEFAULT
#define DRM_MM_TOPDOWN DRM_MM_SEARCH_BELOW, DRM_MM_CREATE_TOP

/**
 * enum drm_mm_insert_mode - control search and allocation behaviour
 *
 * The &struct drm_mm range manager supports finding a suitable modes using
 * a number of search trees. These trees are oranised by size, by address and
 * in most recent eviction order. This allows the user to find either the
 * smallest hole to reuse, the lowest or highest address to reuse, or simply
 * reuse the most recent eviction that fits. When allocating the &drm_mm_node
 * from within the hole, the &drm_mm_insert_mode also dictate whether to
 * allocate the lowest matching address or the highest.
 */
enum drm_mm_insert_mode {
	/**
	 * @DRM_MM_INSERT_BEST:
	 *
	 * Search for the smallest hole (within the search range) that fits
	 * the desired node.
	 *
	 * Allocates the node from the bottom of the found hole.
	 */
	DRM_MM_INSERT_BEST = 0,

	/**
	 * @DRM_MM_INSERT_LOW:
	 *
	 * Search for the lowest hole (address closest to 0, within the search
	 * range) that fits the desired node.
	 *
	 * Allocates the node from the bottom of the found hole.
	 */
	DRM_MM_INSERT_LOW,

	/**
	 * @DRM_MM_INSERT_HIGH:
	 *
	 * Search for the highest hole (address closest to U64_MAX, within the
	 * search range) that fits the desired node.
	 *
	 * Allocates the node from the *top* of the found hole. The specified
	 * alignment for the node is applied to the base of the node
	 * (&drm_mm_node.start).
	 */
	DRM_MM_INSERT_HIGH,

	/**
	 * @DRM_MM_INSERT_EVICT:
	 *
	 * Search for the most recently evicted hole (within the search range)
	 * that fits the desired node. This is appropriate for use immediately
	 * after performing an eviction scan (see drm_mm_scan_init()) and
	 * removing the selected nodes to form a hole.
	 *
	 * Allocates the node from the bottom of the found hole.
	 */
	DRM_MM_INSERT_EVICT,

	/**
	 * @DRM_MM_INSERT_ONCE:
	 *
	 * Only check the first hole for suitablity and report -ENOSPC
	 * immediately otherwise, rather than check every hole until a
	 * suitable one is found. Can only be used in conjunction with another
	 * search method such as DRM_MM_INSERT_HIGH or DRM_MM_INSERT_LOW.
	 */
	DRM_MM_INSERT_ONCE = BIT(31),

	/**
	 * @DRM_MM_INSERT_HIGHEST:
	 *
	 * Only check the highest hole (the hole with the largest address) and
	 * insert the node at the top of the hole or report -ENOSPC if
	 * unsuitable.
	 *
	 * Does not search all holes.
	 */
	DRM_MM_INSERT_HIGHEST = DRM_MM_INSERT_HIGH | DRM_MM_INSERT_ONCE,

	/**
	 * @DRM_MM_INSERT_LOWEST:
	 *
	 * Only check the lowest hole (the hole with the smallest address) and
	 * insert the node at the bottom of the hole or report -ENOSPC if
	 * unsuitable.
	 *
	 * Does not search all holes.
	 */
	DRM_MM_INSERT_LOWEST  = DRM_MM_INSERT_LOW | DRM_MM_INSERT_ONCE,
};

struct drm_mm_node {
	struct list_head node_list;
	struct list_head hole_stack;
	unsigned hole_follows : 1;
	unsigned scanned_block : 1;
	unsigned scanned_prev_free : 1;
	unsigned scanned_next_free : 1;
	unsigned scanned_preceeds_hole : 1;
	unsigned allocated : 1;
	unsigned long color;
	u64 start;
	u64 size;
	struct drm_mm *mm;
};

struct drm_mm {
	/* List of all memory nodes that immediately precede a free hole. */
	struct list_head hole_stack;
	/* head_node.node_list is the list of all memory nodes, ordered
	 * according to the (increasing) start address of the memory node. */
	struct drm_mm_node head_node;

	void (*color_adjust)(const struct drm_mm_node *node, unsigned long color,
			     u64 *start, u64 *end);

	unsigned long scan_active;
};

/**
 * struct drm_mm_scan - DRM allocator eviction roaster data
 *
 * This structure tracks data needed for the eviction roaster set up using
 * drm_mm_scan_init(), and used with drm_mm_scan_add_block() and
 * drm_mm_scan_remove_block(). The structure is entirely opaque and should only
 * be accessed through the provided functions and macros. It is meant to be
 * allocated temporarily by the driver on the stack.
 */
struct drm_mm_scan {
	/* private: */
	struct drm_mm *mm;

	u64 size;
	u64 alignment;
	u64 remainder_mask;

	u64 range_start;
	u64 range_end;

	u64 hit_start;
	u64 hit_end;

	unsigned long color;
	enum drm_mm_insert_mode mode;
};

/**
 * drm_mm_node_allocated - checks whether a node is allocated
 * @node: drm_mm_node to check
 *
 * Drivers should use this helpers for proper encapusulation of drm_mm
 * internals.
 *
 * Returns:
 * True if the @node is allocated.
 */
static inline bool drm_mm_node_allocated(const struct drm_mm_node *node)
{
	return node->allocated;
}

/**
 * drm_mm_initialized - checks whether an allocator is initialized
 * @mm: drm_mm to check
 *
 * Drivers should use this helpers for proper encapusulation of drm_mm
 * internals.
 *
 * Returns:
 * True if the @mm is initialized.
 */
static inline bool drm_mm_initialized(const struct drm_mm *mm)
{
	return mm->hole_stack.next;
}

static inline bool drm_mm_hole_follows(const struct drm_mm_node *node)
{
	return node->hole_follows;
}

static inline u64 __drm_mm_hole_node_start(const struct drm_mm_node *hole_node)
{
	return hole_node->start + hole_node->size;
}

/**
 * drm_mm_hole_node_start - computes the start of the hole following @node
 * @hole_node: drm_mm_node which implicitly tracks the following hole
 *
 * This is useful for driver-sepific debug dumpers. Otherwise drivers should not
 * inspect holes themselves. Drivers must check first whether a hole indeed
 * follows by looking at node->hole_follows.
 *
 * Returns:
 * Start of the subsequent hole.
 */
static inline u64 drm_mm_hole_node_start(const struct drm_mm_node *hole_node)
{
	BUG_ON(!hole_node->hole_follows);
	return __drm_mm_hole_node_start(hole_node);
}

static inline u64 __drm_mm_hole_node_end(const struct drm_mm_node *hole_node)
{
	return list_next_entry(hole_node, node_list)->start;
}

/**
 * drm_mm_hole_node_end - computes the end of the hole following @node
 * @hole_node: drm_mm_node which implicitly tracks the following hole
 *
 * This is useful for driver-sepific debug dumpers. Otherwise drivers should not
 * inspect holes themselves. Drivers must check first whether a hole indeed
 * follows by looking at node->hole_follows.
 *
 * Returns:
 * End of the subsequent hole.
 */
static inline u64 drm_mm_hole_node_end(const struct drm_mm_node *hole_node)
{
	return __drm_mm_hole_node_end(hole_node);
}

/**
 * drm_mm_for_each_node - iterator to walk over all allocated nodes
 * @entry: drm_mm_node structure to assign to in each iteration step
 * @mm: drm_mm allocator to walk
 *
 * This iterator walks over all nodes in the range allocator. It is implemented
 * with list_for_each, so not save against removal of elements.
 */
#define drm_mm_for_each_node(entry, mm) list_for_each_entry(entry, \
						&(mm)->head_node.node_list, \
						node_list)

#define __drm_mm_for_each_hole(entry, mm, hole_start, hole_end, backwards) \
	for (entry = list_entry((backwards) ? (mm)->hole_stack.prev : (mm)->hole_stack.next, struct drm_mm_node, hole_stack); \
	     &entry->hole_stack != &(mm)->hole_stack ? \
	     hole_start = drm_mm_hole_node_start(entry), \
	     hole_end = drm_mm_hole_node_end(entry), \
	     1 : 0; \
	     entry = list_entry((backwards) ? entry->hole_stack.prev : entry->hole_stack.next, struct drm_mm_node, hole_stack))

/**
 * drm_mm_for_each_hole - iterator to walk over all holes
 * @entry: drm_mm_node used internally to track progress
 * @mm: drm_mm allocator to walk
 * @hole_start: ulong variable to assign the hole start to on each iteration
 * @hole_end: ulong variable to assign the hole end to on each iteration
 *
 * This iterator walks over all holes in the range allocator. It is implemented
 * with list_for_each, so not save against removal of elements. @entry is used
 * internally and will not reflect a real drm_mm_node for the very first hole.
 * Hence users of this iterator may not access it.
 *
 * Implementation Note:
 * We need to inline list_for_each_entry in order to be able to set hole_start
 * and hole_end on each iteration while keeping the macro sane.
 *
 * The __drm_mm_for_each_hole version is similar, but with added support for
 * going backwards.
 */
#define drm_mm_for_each_hole(entry, mm, hole_start, hole_end) \
	__drm_mm_for_each_hole(entry, mm, hole_start, hole_end, 0)

/*
 * Basic range manager support (drm_mm.c)
 */
int drm_mm_reserve_node(struct drm_mm *mm, struct drm_mm_node *node);

int drm_mm_insert_node_generic(struct drm_mm *mm,
			       struct drm_mm_node *node,
			       u64 size,
			       unsigned alignment,
			       unsigned long color,
			       enum drm_mm_search_flags sflags,
			       enum drm_mm_allocator_flags aflags);
/**
 * drm_mm_insert_node - search for space and insert @node
 * @mm: drm_mm to allocate from
 * @node: preallocate node to insert
 * @size: size of the allocation
 * @alignment: alignment of the allocation
 * @flags: flags to fine-tune the allocation
 *
 * This is a simplified version of drm_mm_insert_node_generic() with @color set
 * to 0.
 *
 * The preallocated node must be cleared to 0.
 *
 * Returns:
 * 0 on success, -ENOSPC if there's no suitable hole.
 */
static inline int drm_mm_insert_node(struct drm_mm *mm,
				     struct drm_mm_node *node,
				     u64 size,
				     unsigned alignment,
				     enum drm_mm_search_flags flags)
{
	return drm_mm_insert_node_generic(mm, node, size, alignment, 0, flags,
					  DRM_MM_CREATE_DEFAULT);
}

int drm_mm_insert_node_in_range_generic(struct drm_mm *mm,
					struct drm_mm_node *node,
					u64 size,
					unsigned alignment,
					unsigned long color,
					u64 start,
					u64 end,
					enum drm_mm_search_flags sflags,
					enum drm_mm_allocator_flags aflags);
/**
 * drm_mm_insert_node_in_range - ranged search for space and insert @node
 * @mm: drm_mm to allocate from
 * @node: preallocate node to insert
 * @size: size of the allocation
 * @alignment: alignment of the allocation
 * @start: start of the allowed range for this node
 * @end: end of the allowed range for this node
 * @flags: flags to fine-tune the allocation
 *
 * This is a simplified version of drm_mm_insert_node_in_range_generic() with
 * @color set to 0.
 *
 * The preallocated node must be cleared to 0.
 *
 * Returns:
 * 0 on success, -ENOSPC if there's no suitable hole.
 */
static inline int drm_mm_insert_node_in_range(struct drm_mm *mm,
					      struct drm_mm_node *node,
					      u64 size,
					      unsigned alignment,
					      unsigned long color,
					      u64 start,
					      u64 end,
					      enum drm_mm_insert_mode mode)
{
	enum drm_mm_search_flags sflags;
	enum drm_mm_allocator_flags aflags;
	switch (mode) {
	case DRM_MM_INSERT_HIGHEST:
		sflags = DRM_MM_SEARCH_BELOW;
		aflags = DRM_MM_CREATE_TOP;
		break;
	case DRM_MM_INSERT_BEST:
		sflags = DRM_MM_SEARCH_BEST;
		aflags = DRM_MM_CREATE_DEFAULT;
		break;
	case DRM_MM_INSERT_LOW:
	case DRM_MM_INSERT_HIGH:
	case DRM_MM_INSERT_EVICT:
	case DRM_MM_INSERT_ONCE:
	case DRM_MM_INSERT_LOWEST:
	default:
		sflags = DRM_MM_SEARCH_DEFAULT; 
		aflags = DRM_MM_CREATE_DEFAULT;
		break;
	}
	return drm_mm_insert_node_in_range_generic(mm, node, size, alignment,
						   color, start, end,
						   sflags, aflags);
}

void drm_mm_remove_node(struct drm_mm_node *node);
void drm_mm_replace_node(struct drm_mm_node *old, struct drm_mm_node *new);
void drm_mm_init(struct drm_mm *mm, u64 start, u64 size);
void drm_mm_takedown(struct drm_mm *mm);
bool drm_mm_clean(const struct drm_mm *mm);

void drm_mm_scan_init_with_range(struct drm_mm_scan *scan,
				 struct drm_mm *mm,
				 u64 size, u64 alignment, unsigned long color,
				 u64 start, u64 end,
				 enum drm_mm_insert_mode mode);

/**
 * drm_mm_scan_init - initialize lru scanning
 * @scan: scan state
 * @mm: drm_mm to scan
 * @size: size of the allocation
 * @alignment: alignment of the allocation
 * @color: opaque tag value to use for the allocation
 * @mode: fine-tune the allocation search and placement
 *
 * This is a simplified version of drm_mm_scan_init_with_range() with no range
 * restrictions applied.
 *
 * This simply sets up the scanning routines with the parameters for the desired
 * hole.
 *
 * Warning:
 * As long as the scan list is non-empty, no other operations than
 * adding/removing nodes to/from the scan list are allowed.
 */
static inline void drm_mm_scan_init(struct drm_mm_scan *scan,
				    struct drm_mm *mm,
				    u64 size,
				    u64 alignment,
				    unsigned long color,
				    enum drm_mm_insert_mode mode)
{
	drm_mm_scan_init_with_range(scan, mm,
				    size, alignment, color,
				    0, U64_MAX, mode);
}

bool drm_mm_scan_add_block(struct drm_mm_scan *scan,
			   struct drm_mm_node *node);
bool drm_mm_scan_remove_block(struct drm_mm_scan *scan,
			      struct drm_mm_node *node);
struct drm_mm_node *drm_mm_scan_color_evict(struct drm_mm_scan *scan);

void drm_mm_debug_table(const struct drm_mm *mm, const char *prefix);
#ifdef CONFIG_DEBUG_FS
int drm_mm_dump_table(struct seq_file *m, const struct drm_mm *mm);
#endif

#endif
