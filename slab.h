/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _NAVI_SLAB_H_
#define _NAVI_SLAB_H_

#include "rbtree.h"

enum {
	NAVI_SLAB_DOUBLY_LINKED,
};

struct slab;

struct slab_cache {
	struct rb_tree full;
	struct rb_tree partial;
	struct rb_tree empty;
	unsigned int flags;
	unsigned int objs_per_slab;
	size_t obj_size;
};

struct slab_cache *navi_slab_cache_create(size_t size, unsigned int flags);
void *navi_slab_alloc(struct slab_cache *cache);
void navi_slab_free(struct slab_cache *cache, void *mem);

#endif
