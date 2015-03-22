/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "slab.h"

/*
 * Slab Allocator: memory allocator for small, fixed-size objects.
 *
 * Each type of object has an associated "slab cache."  A slab cache is a
 * collection of "slabs," which are chunks of memory from which objects are
 * allocated.
 *
 * This allocator reduces fragmentation and speeds up alloc/free.
 */

#define SLAB_SIZE 4096
#define SLAB_DESC_SIZE offsetof(struct slab, mem)
#define SLAB_MEM_SIZE (SLAB_SIZE - SLAB_DESC_SIZE)

struct slab_cache *navi_slab_cache_create(size_t size)
{
	struct slab_cache *cache = navi_critical_malloc(sizeof(struct slab_cache));

	NAVI_LIST_INIT(&cache->full);
	NAVI_LIST_INIT(&cache->partial);
	NAVI_LIST_INIT(&cache->empty);

	cache->obj_size = (size < 16) ? 16 : size;
	cache->objs_per_slab = SLAB_MEM_SIZE / cache->obj_size;

	return cache;
}

__const void *get_object(struct slab_cache *cache, struct slab *slab, unsigned i)
{
	return (void*)((uintptr_t)slab->mem + cache->obj_size*i);
}

/* Allocates and initializes a new slab for the given cache */
static struct slab *new_slab(struct slab_cache *cache)
{
	struct slab *slab = navi_critical_malloc(SLAB_SIZE);

	NAVI_SLIST_INIT(&slab->free);
	for (unsigned i = 0; i < cache->objs_per_slab; i++) {
		struct navi_object *obj = get_object(cache, slab, i);
		NAVI_SLIST_INSERT_HEAD(&slab->free, obj, link);
	}

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

__hot void *navi_slab_alloc(struct slab_cache *cache)
{
	struct slab *slab = get_slab(cache);
	struct navi_object *obj = NAVI_SLIST_FIRST(&slab->free);
	NAVI_SLIST_REMOVE_HEAD(&slab->free, link);

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

	NAVI_SLIST_INSERT_HEAD(&slab->free, (struct navi_object*)mem, link);

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
