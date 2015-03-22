/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _NAVI_SLAB_H_
#define _NAVI_SLAB_H_

struct slab {
	NAVI_LIST_ENTRY(slab) link;
	NAVI_SLIST_HEAD(slab_head, navi_object) free;
	unsigned int in_use;
	unsigned long mem[];
};

struct slab_cache {
	NAVI_LIST_HEAD(sc_full, slab) full;
	NAVI_LIST_HEAD(sc_partial, slab) partial;
	NAVI_LIST_HEAD(sc_empty, slab) empty;
	unsigned int objs_per_slab;
	size_t obj_size;
};

struct slab_cache *navi_slab_cache_create(size_t size);
void *navi_slab_alloc(struct slab_cache *cache);
void navi_slab_free(struct slab_cache *cache, void *mem);

#endif
