/* Copyright 2014-2015 Drew Thoreson
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stddef.h>
#include "rbtree.h"

static inline int is_left_child(struct rb_node *n)
{
	return n == n->parent->left;
}

static inline int is_right_child(struct rb_node *n)
{
	return n == n->parent->right;
}

static inline int is_red(struct rb_node *n)
{
	return n && n->color == RB_RED;
}

static inline int is_black(struct rb_node *n)
{
	return !n || n->color == RB_BLACK;
}

static inline enum rb_color rb_color(struct rb_node *node)
{
	return node ? node->color : RB_BLACK;
}

#define is_root(n)  ((n)->parent->color == RB_ROOT_MARK)

static struct rb_node *rb_uncle(struct rb_node *n)
{
	struct rb_node *grandparent = n->parent->parent;
	if (n->parent == grandparent->left)
		return grandparent->right;
	return grandparent->left;
}

static struct rb_node *rb_sibling(struct rb_node *n)
{
	if (is_left_child(n))
		return n->parent->right;
	return n->parent->left;
}

static void replace(struct rb_node *old, struct rb_node *new)
{
	if (is_left_child(old))
		old->parent->left = new;
	else
		old->parent->right = new;
	if (new)
		new->parent = old->parent;
}

static void rotate_left(struct rb_node *parent)
{
	struct rb_node *right = parent->right;
	replace(parent, right);
	parent->right = right->left;
	if (right->left)
		right->left->parent = parent;
	right->left = parent;
	parent->parent = right;
}

static void rotate_right(struct rb_node *parent)
{
	struct rb_node *left = parent->left;
	replace(parent, left);
	parent->left = left->right;
	if (left->right)
		left->right->parent = parent;
	left->right = parent;
	parent->parent = left;
}

void navi_rb_insert(struct rb_node *new, struct rb_node *parent,
		struct rb_node **parent_link)
{
	new->left = NULL;
	new->right = NULL;
	new->color = RB_RED;
	new->parent = parent;
	*parent_link = new;

	for (;;) {
		// case 1:
		if (is_root(new)) {
			new->color = RB_BLACK;
			return;
		}

		// case 2:
		if (is_black(new->parent))
			return;

		// case 3:
		struct rb_node *uncle = rb_uncle(new);
		if (uncle && is_red(uncle)) {
			new->parent->color = RB_BLACK;
			uncle->color = RB_BLACK;
			new->parent->parent->color = RB_RED;

			// XXX: recursion
			new = new->parent->parent;
			continue;
		}

		// case 4:
		if (is_right_child(new) && is_left_child(new->parent)) {
			rotate_left(new->parent);
			new = new->left;
		} else if (is_left_child(new) && is_right_child(new->parent)) {
			rotate_right(new->parent);
			new = new->right;
		}

		// case 5:
		new->parent->color = RB_BLACK;
		new->parent->parent->color = RB_RED;
		if (is_left_child(new) && is_left_child(new->parent))
			rotate_right(new->parent->parent);
		else
			rotate_left(new->parent->parent);
	}
}

struct rb_node *navi_rb_first(struct rb_node *node)
{
	if (!node)
		return NULL;
	while (node->left)
		node = node->left;
	return node;
}

struct rb_node *navi_rb_last(struct rb_node *node)
{
	if (!node)
		return NULL;
	while (node->right)
		node = node->right;
	return node;
}

struct rb_node *navi_rb_next(struct rb_node *node)
{
	if (node->right)
		return navi_rb_first(node->right);
	while (is_right_child(node))
		node = node->parent;
	if (is_root(node))
		return NULL;
	return node->parent;
}

static void swap(struct rb_node *a, struct rb_node *b)
{
	struct rb_node _a = *a;

	// swap a children
	a->left = b->left;
	if (b->left)
		b->left->parent = a;
	a->right = b->right;
	if (b->right)
		b->right->parent = a;

	// swap b children
	b->left = _a.left;
	if (_a.left)
		_a.left->parent = b;
	b->right = _a.right;
	if (_a.right)
		_a.right->parent = b;

	// swap a parent
	if (is_left_child(a))
		a->parent->left = b;
	else
		a->parent->right = b;
	a->parent = b->parent;

	// swap b parent
	if (is_left_child(b))
		b->parent->left = a;
	else
		b->parent->right = a;
	b->parent = _a.parent;

	// swap colors
	a->color = b->color;
	b->color = _a.color;
}

void navi_rb_remove(struct rb_node *n)
{
	// if n has two children, swap n with pred(n): then n has at most
	// one child (the left child).
	if (n->left && n->right)
		swap(n, navi_rb_last(n->left));

	struct rb_node *saved_n = n;
	struct rb_node *child = n->right ? n->right : n->left;
	if (is_black(n)) {
		n->color = rb_color(child);
		for (;;) {
			// case 1:
			if (is_root(n))
				break;

			// case 2:
			struct rb_node *sibling = rb_sibling(n);
			if (is_red(sibling)) {
				n->parent->color = RB_RED;
				sibling->color = RB_BLACK;
				if (is_left_child(n))
					rotate_left(n->parent);
				else
					rotate_right(n->parent);
			}

			// case 3:
			sibling = rb_sibling(n);
			if (!sibling)
				break;
			if (is_black(n->parent) && is_black(sibling)
					&& is_black(sibling->left)
					&& is_black(sibling->right)) {
				sibling->color = RB_RED;

				// XXX: recursion
				n = n->parent;
				continue;
			}

			// case 4:
			if (is_red(n->parent) && is_black(sibling)
					&& is_black(sibling->left)
					&& is_black(sibling->right)) {
				sibling->color = RB_RED;
				n->parent->color = RB_BLACK;
				break;
			}

			// case 5:
			if (is_left_child(n) && is_black(sibling)
					&& is_red(sibling->left)
					&& is_black(sibling->right)) {
				sibling->color = RB_RED;
				sibling->left->color = RB_BLACK;
				rotate_right(sibling);
			} else if (is_right_child(n) && is_black(sibling)
					&& is_red(sibling->right)
					&& is_black(sibling->left)) {
				sibling->color = RB_RED;
				sibling->right->color = RB_BLACK;
				rotate_left(sibling);
			}

			// case 6:
			sibling = rb_sibling(n);
			sibling->color = rb_color(n->parent);
			n->parent->color = RB_BLACK;
			if (is_left_child(n)) {
				sibling->right->color = RB_BLACK;
				rotate_left(n->parent);
			} else {
				sibling->left->color = RB_BLACK;
				rotate_right(n->parent);
			}
			break;
		}
	}
	replace(saved_n, child);
	if (is_root(n) && child)
		child->color = RB_BLACK;
}
