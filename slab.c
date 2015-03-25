/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * Slab Allocator: memory allocator for small, fixed-size objects.
 *
 * Each type of object has an associated "slab cache."  A slab cache is a
 * collection of "slabs," which are chunks of memory from which objects are
 * allocated.
 *
 * This allocator reduces fragmentation and speeds up alloc/free.
 */

#include "slab.h"

#define SLAB_SIZE 4096
#define SLAB_DESC_SIZE offsetof(struct slab, mem)
#define SLAB_MEM_SIZE (SLAB_SIZE - SLAB_DESC_SIZE)

/*
 * Some objects that use the slab allocator have NAVI_SLIST_ENTRYs, while
 * other's have NAVI_LIST_ENTRYs -- but all of them have their link(s)
 * as the first member of their struct.
 *
 * We're playing fast and loose with pointers here, and probably breaking the
 * strict aliasing rule.  Let's hope the compiler doesn't do some crazy LTO
 * and break this.
 */

struct slab_slist_entry {
	NAVI_SLIST_ENTRY(slab_slist_entry) link;
	unsigned char data[];
};

struct slab_list_entry {
	NAVI_LIST_ENTRY(slab_list_entry) link;
	unsigned char data[];
};

struct slab {
	NAVI_LIST_ENTRY(slab) link;
	union {
		NAVI_SLIST_HEAD(slab_slist_head, slab_slist_entry) free;
		NAVI_LIST_HEAD(slab_list_head, slab_list_entry) _free;
	};
	unsigned int in_use;
	unsigned long mem[];
};

struct slab_cache *navi_slab_cache_create(size_t size, unsigned int flags)
{
	struct slab_cache *cache = navi_critical_malloc(sizeof(struct slab_cache));

	NAVI_LIST_INIT(&cache->full);
	NAVI_LIST_INIT(&cache->partial);
	NAVI_LIST_INIT(&cache->empty);

	cache->flags = flags;
	cache->obj_size = (size < 16) ? 16 : size;
	cache->objs_per_slab = SLAB_MEM_SIZE / cache->obj_size;

	return cache;
}

__const void *get_object(struct slab_cache *cache, struct slab *slab, unsigned i)
{
	return (void*)((uintptr_t)slab->mem + cache->obj_size*i);
}

static void init_single(struct slab_cache *cache, struct slab *slab)
{
	NAVI_SLIST_INIT(&slab->free);
	for (unsigned i = 0; i < cache->objs_per_slab; i++) {
		struct slab_slist_entry *obj = get_object(cache, slab, i);
		NAVI_SLIST_INSERT_HEAD(&slab->free, obj, link);
	}
}

static void init_double(struct slab_cache *cache, struct slab *slab)
{
	NAVI_LIST_INIT(&slab->_free);
	for (unsigned i = 0; i < cache->objs_per_slab; i++) {
		struct slab_list_entry *obj = get_object(cache, slab, i);
		NAVI_LIST_INSERT_HEAD(&slab->_free, obj, link);
	}
}

/* Allocates and initializes a new slab for the given cache */
static struct slab *new_slab(struct slab_cache *cache)
{
	struct slab *slab = navi_critical_malloc(SLAB_SIZE);

	if (cache->flags & NAVI_SLAB_DOUBLY_LINKED)
		init_double(cache, slab);
	else
		init_single(cache, slab);

	slab->in_use = 0;
	return slab;
}

static struct slab *get_slab(struct slab_cache *cache)
{
	struct slab *slab;

	if (NAVI_LIST_EMPTY(&cache->partial) && NAVI_LIST_EMPTY(&cache->empty)) {
		slab = new_slab(cache);
		NAVI_LIST_INSERT_HEAD(&cache->partial, slab, link);
		return slab;
	}

	if (NAVI_LIST_EMPTY(&cache->partial))
		return NAVI_LIST_FIRST(&cache->empty);

	return NAVI_LIST_FIRST(&cache->partial);
}

static void *slab_pop(struct slab_cache *cache, struct slab *slab)
{
	if (cache->flags & NAVI_SLAB_DOUBLY_LINKED) {
		struct slab_list_entry *obj = NAVI_LIST_FIRST(&slab->_free);
		NAVI_LIST_REMOVE(obj, link);
		return obj;
	}
	struct slab_slist_entry *obj = NAVI_SLIST_FIRST(&slab->free);
	NAVI_SLIST_REMOVE_HEAD(&slab->free, link);
	return obj;
}

__hot void *navi_slab_alloc(struct slab_cache *cache)
{
	struct slab *slab = get_slab(cache);
	void *obj = slab_pop(cache, slab);

	// move slab to full/partial list, as appropriate
	if (++slab->in_use == cache->objs_per_slab) {
		NAVI_LIST_REMOVE(slab, link);
		NAVI_LIST_INSERT_HEAD(&cache->full, slab, link);
	} else if (slab->in_use == 1) {
		NAVI_LIST_REMOVE(slab, link);
		NAVI_LIST_INSERT_HEAD(&cache->partial, slab, link);
	}
	return obj;
}

static inline __const void *slab_end(struct slab_cache *cache, struct slab *slab)
{
	return (void*) ((uintptr_t)slab->mem + SLAB_MEM_SIZE);
}

/*
 * TODO: Use trees for full/partial/empty lists for logarithmic lookup time.
 */
static __hot __const struct slab *find_slab(struct slab_cache *cache, void *mem)
{
	struct slab *slab;
	NAVI_LIST_FOREACH(slab, &cache->full, link) {
		if (mem >= (void*)slab->mem && mem < slab_end(cache, slab))
			return slab;
	}
	NAVI_LIST_FOREACH(slab, &cache->partial, link) {
		if (mem >= (void*)slab->mem && mem < slab_end(cache, slab))
			return slab;
	}
	return NULL;
}

void __hot navi_slab_free(struct slab_cache *cache, void *mem)
{
	struct slab *slab;
	if (!(slab = find_slab(cache, mem)))
		navi_die("slab_free: failed to locate slab!\n");

	if (cache->flags & NAVI_SLAB_DOUBLY_LINKED) {
		struct slab_list_entry *obj = mem;
		NAVI_LIST_INSERT_HEAD(&slab->_free, obj, link);
	} else {
		struct slab_slist_entry *obj = mem;
		NAVI_SLIST_INSERT_HEAD(&slab->free, obj, link);
	}

	// update slab status within cache
	if (--slab->in_use == 0) {
		NAVI_LIST_REMOVE(slab, link);
		NAVI_LIST_INSERT_HEAD(&cache->empty, slab, link);
	} else if (slab->in_use + 1 == cache->objs_per_slab) {
		NAVI_LIST_REMOVE(slab, link);
		NAVI_LIST_INSERT_HEAD(&cache->partial, slab, link);
	}
}

/*
 * TODO: navi_slab_shrink(struct slab_cache *cache)
 *
 * Function to try reclaiming memory from a slab cache.
 */
