/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _NAVI_RB_TREE_H_
#define _NAVI_RB_TREE_H_

#include <stddef.h>

/*
 * Generic Red-Black Trees
 *
 * Red-black tree implementation using embedded links.  The interface is
 * inspired by the Linux kernel implementation.  Search (including for
 * insertion and deletion) must be implemented per-object type.  e.g. a
 * typical insert routine looks something like:
 *
 * static struct data *data_insert(struct rb_tree *tree, struct data *data)
 * {
 *	struct data *ptr;
 *	struct rb_node **p = rb_root_link(tree);
 *	struct rb_node *parent = rb_root_parent(tree);
 *
 *	while (*p) {
 *		parent = *p;
 *		ptr = rb_entry(parent, struct data, node);
 *
 *		if (data->data < ptr->data)
 *			p = &(*p)->left;
 *		else if (data->data > ptr->data)
 *			p = &(*p)->right;
 *		else
 *			return ptr;
 *	}
 *
 *	rb_insert(&slab->node, parent, p);
 *	return NULL;
 * }
 */

enum rb_color {
	RB_RED,
	RB_BLACK,
	RB_ROOT_MARK
};

struct rb_node {
	struct rb_node *left;
	struct rb_node *right;
	struct rb_node *parent;
	enum rb_color color;
};

struct rb_tree {
	struct rb_node root; // dummy root; not part of the tree
};

#define RB_ROOT_INIT { .root = { .color = RB_ROOT_MARK } }

#define RB_ROOT(name) struct rb_tree name = RB_ROOT_INIT

static inline struct rb_node *rb_root(struct rb_tree *tree)
{
	return tree->root.left;
}

static inline struct rb_node *rb_root_parent(struct rb_tree *tree)
{
	return &tree->root;
}

static inline struct rb_node **rb_root_link(struct rb_tree *tree)
{
	return &tree->root.left;
}

static inline int rb_empty(struct rb_tree *tree)
{
	return !rb_root(tree);
}

void navi_rb_insert(struct rb_node *new, struct rb_node *parent,
		struct rb_node **parent_link);
void navi_rb_remove(struct rb_node *n);

struct rb_node *navi_rb_first(struct rb_node *node);
struct rb_node *navi_rb_last(struct rb_node *node);
struct rb_node *navi_rb_next(struct rb_node *node);

#define rb_entry(ptr, type, member) \
	((type*) ((char*)(ptr) - offsetof(type, member)))

#define rb_for_each(n, tree) \
	for (n = navi_rb_first(rb_root(tree)); n; n = navi_rb_next(n))

#endif
