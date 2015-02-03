/*
 * Stolen from Linux 3.11-rc7
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _NAVI_CLIST_H
#define _NAVI_CLIST_H

#include <stddef.h> /* NULL, offsetof */

#ifdef __GNUC__

#if __GNUC__ > 3
#undef offsetof
#define offsetof(type, member) __builtin_offsetof(type, member)
#endif

#define container_of(ptr, type, member) ({ 				\
		const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
		(type *)( (char *)__mptr - offsetof(type, member) );})

#else /* use portable container_of */

#define container_of(ptr, type, member)					\
	((type *)(void *)( (char *)(ptr) - offsetof(type, member) ))

#endif /* __GNUC__ */

/*
 * These are non-NULL pointers that will result in page faults
 * under normal circumstances, used to verify that nobody uses
 * non-initialized list entries.
 */
#define NAVI_LIST_POISON1 ((void *) 0x00100100)
#define NAVI_LIST_POISON2 ((void *) 0x00200200)

struct navi_clist_head {
	struct navi_clist_head *next, *prev;
};

struct navi_hlist_head {
	struct navi_hlist_node *first;
};

struct navi_hlist_node {
	struct navi_hlist_node *next, **pprev;
};

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

#define NAVI_LIST_HEAD_INIT(name) { &(name), &(name) }

#define NAVI_LIST_HEAD(name) \
	struct navi_clist_head name = NAVI_LIST_HEAD_INIT(name)

static inline void NAVI_INIT_LIST_HEAD(struct navi_clist_head *list)
{
	list->next = list;
	list->prev = list;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __navi_clist_add(struct navi_clist_head *new,
				   struct navi_clist_head *prev,
				   struct navi_clist_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

/**
 * navi_clist_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void navi_clist_add(struct navi_clist_head *new, struct navi_clist_head *head)
{
	__navi_clist_add(new, head, head->next);
}


/**
 * navi_clist_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void navi_clist_add_tail(struct navi_clist_head *new, struct navi_clist_head *head)
{
	__navi_clist_add(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __navi_clist_del(struct navi_clist_head * prev,
				   struct navi_clist_head * next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * navi_clist_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: navi_clist_empty() on entry does not return true after this, the entry
 * is in an undefined state.
 */
static inline void __navi_clist_del_entry(struct navi_clist_head *entry)
{
	__navi_clist_del(entry->prev, entry->next);
}

static inline void navi_clist_del(struct navi_clist_head *entry)
{
	__navi_clist_del(entry->prev, entry->next);
	entry->next = NAVI_LIST_POISON1;
	entry->prev = NAVI_LIST_POISON2;
}

/**
 * navi_clist_replace - replace old entry by new one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void navi_clist_replace(struct navi_clist_head *old,
				     struct navi_clist_head *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}

static inline void navi_clist_replace_init(struct navi_clist_head *old,
					  struct navi_clist_head *new)
{
	navi_clist_replace(old, new);
	NAVI_INIT_LIST_HEAD(old);
}

/**
 * navi_clist_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void navi_clist_del_init(struct navi_clist_head *entry)
{
	__navi_clist_del_entry(entry);
	NAVI_INIT_LIST_HEAD(entry);
}

/**
 * navi_clist_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static inline void navi_clist_move(struct navi_clist_head *list,
				  struct navi_clist_head *head)
{
	__navi_clist_del_entry(list);
	navi_clist_add(list, head);
}

/**
 * navi_clist_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static inline void navi_clist_move_tail(struct navi_clist_head *list,
				       struct navi_clist_head *head)
{
	__navi_clist_del_entry(list);
	navi_clist_add_tail(list, head);
}

/**
 * navi_clist_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int navi_clist_is_last(const struct navi_clist_head *list,
				    const struct navi_clist_head *head)
{
	return list->next == head;
}

/**
 * navi_clist_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int navi_clist_empty(const struct navi_clist_head *head)
{
	return head->next == head;
}

/**
 * navi_clist_empty_careful - tests whether a list is empty and not being
 * modified
 * @head: the list to test
 *
 * Description:
 * tests whether a list is empty _and_ checks that no other CPU might be
 * in the process of modifying either member (next or prev)
 *
 * NOTE: using navi_clist_empty_careful() without synchronization
 * can only be safe if the only activity that can happen
 * to the list entry is list_del_init(). Eg. it cannot be used
 * if another CPU could re-list_add() it.
 */
static inline int navi_clist_empty_careful(const struct navi_clist_head *head)
{
	struct navi_clist_head *next = head->next;
	return (next == head) && (next == head->prev);
}

/**
 * navi_clist_rotate_left - rotate the list to the left
 * @head: the head of the list
 */
static inline void navi_clist_rotate_left(struct navi_clist_head *head)
{
	struct navi_clist_head *first;

	if (!navi_clist_empty(head)) {
		first = head->next;
		navi_clist_move_tail(first, head);
	}
}

/**
 * navi_clist_is_singular - tests whether a list has just one entry.
 * @head: the list to test.
 */
static inline int navi_clist_is_singular(const struct navi_clist_head *head)
{
	return !navi_clist_empty(head) && (head->next == head->prev);
}

static inline void __navi_clist_cut_position(struct navi_clist_head *list,
		struct navi_clist_head *head, struct navi_clist_head *entry)
{
	struct navi_clist_head *new_first = entry->next;
	list->next = head->next;
	list->next->prev = list;
	list->prev = entry;
	entry->next = list;
	head->next = new_first;
	new_first->prev = head;
}

/**
 * navi_clist_cut_position - cut a list into two
 * @list: a new list to add all removed entries
 * @head: a list with entries
 * @entry: an entry within head, could be the head itself
 *	and if so we won't cut the list
 *
 * This helper moves the initial part of @head, up to and
 * including @entry, from @head to @list. You should
 * pass on @entry an element you know is on @head. @list
 * should be an empty list or a list you do not care about
 * losing its data.
 *
 */
static inline void navi_clist_cut_position(struct navi_clist_head *list,
		struct navi_clist_head *head, struct navi_clist_head *entry)
{
	if (navi_clist_empty(head))
		return;
	if (navi_clist_is_singular(head) &&
		(head->next != entry && head != entry))
		return;
	if (entry == head)
		NAVI_INIT_LIST_HEAD(list);
	else
		__navi_clist_cut_position(list, head, entry);
}

static inline void __navi_clist_splice(const struct navi_clist_head *list,
				      struct navi_clist_head *prev,
				      struct navi_clist_head *next)
{
	struct navi_clist_head *first = list->next;
	struct navi_clist_head *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

/**
 * navi_clist_splice - join two lists, this is designed for stacks
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void navi_clist_splice(const struct navi_clist_head *list,
				    struct navi_clist_head *head)
{
	if (!navi_clist_empty(list))
		__navi_clist_splice(list, head, head->next);
}

/**
 * navi_clist_splice_tail - join two lists, each list being a queue
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void navi_clist_splice_tail(struct navi_clist_head *list,
					 struct navi_clist_head *head)
{
	if (!navi_clist_empty(list))
		__navi_clist_splice(list, head->prev, head);
}

/**
 * navi_clist_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
static inline void navi_clist_splice_init(struct navi_clist_head *list,
					 struct navi_clist_head *head)
{
	if (!navi_clist_empty(list)) {
		__navi_clist_splice(list, head, head->next);
		NAVI_INIT_LIST_HEAD(list);
	}
}

/**
 * navi_clist_splice_tail_init - join two lists and reinitialise the emptied list
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * Each of the lists is a queue.
 * The list at @list is reinitialised
 */
static inline void navi_clist_splice_tail_init(struct navi_clist_head *list,
					      struct navi_clist_head *head)
{
	if (!navi_clist_empty(list)) {
		__navi_clist_splice(list, head->prev, head);
		NAVI_INIT_LIST_HEAD(list);
	}
}

/**
 * navi_clist_entry - get the struct for this entry
 * @ptr:	the &struct navi_clist_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define navi_clist_entry(ptr, type, member) \
	container_of(ptr, type, member)

/**
 * navi_clist_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define navi_clist_first_entry(ptr, type, member) \
	navi_clist_entry((ptr)->next, type, member)

/**
 * navi_clist_first_entry_or_null - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 *
 * Note that if the list is empty, it returns NULL.
 */
#define navi_clist_first_entry_or_null(ptr, type, member) \
	(!navi_clist_empty(ptr) ? navi_clist_first_entry(ptr, type, member) : NULL)

/**
 * navi_clist_for_each	-	iterate over a list
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define navi_clist_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * navi_clist_for_each_prev	-	iterate over a list backwards
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define navi_clist_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * navi_clist_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:	the &struct list_head to use as a loop cursor.
 * @n:		another &struct list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define navi_clist_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/**
 * navi_clist_for_each_prev_safe - iterate over a list backwards safe against removal of list entry
 * @pos:	the &struct list_head to use as a loop cursor.
 * @n:		another &struct list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define navi_clist_for_each_prev_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; \
	     pos != (head); \
	     pos = n, n = pos->prev)

/**
 * navi_clist_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define navi_clist_for_each_entry(pos, head, member)				\
	for (pos = navi_clist_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = navi_clist_entry(pos->member.next, typeof(*pos), member))

/**
 * navi_clist_for_each_entry_reverse - iterate backwards over list of given type.
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define navi_clist_for_each_entry_reverse(pos, head, member)			\
	for (pos = navi_clist_entry((head)->prev, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = navi_clist_entry(pos->member.prev, typeof(*pos), member))

/**
 * navi_clist_prepare_entry - prepare a pos entry for use in navi_clist_for_each_entry_continue()
 * @pos:	the type * to use as a start point
 * @head:	the head of the list
 * @member:	the name of the list_struct within the struct.
 *
 * Prepares a pos entry for use as a start point in navi_clist_for_each_entry_continue().
 */
#define navi_clist_prepare_entry(pos, head, member) \
	((pos) ? : navi_clist_entry(head, typeof(*pos), member))

/**
 * navi_clist_for_each_entry_continue - continue iteration over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Continue to iterate over list of given type, continuing after
 * the current position.
 */
#define navi_clist_for_each_entry_continue(pos, head, member) 		\
	for (pos = navi_clist_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head);	\
	     pos = navi_clist_entry(pos->member.next, typeof(*pos), member))

/**
 * navi_clist_for_each_entry_continue_reverse - iterate backwards from the given point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Start to iterate over list of given type backwards, continuing after
 * the current position.
 */
#define navi_clist_for_each_entry_continue_reverse(pos, head, member)		\
	for (pos = navi_clist_entry(pos->member.prev, typeof(*pos), member);	\
	     &pos->member != (head);	\
	     pos = navi_clist_entry(pos->member.prev, typeof(*pos), member))

/**
 * navi_clist_for_each_entry_from - iterate over list of given type from the current point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate over list of given type, continuing from current position.
 */
#define navi_clist_for_each_entry_from(pos, head, member) 			\
	for (; &pos->member != (head);	\
	     pos = navi_clist_entry(pos->member.next, typeof(*pos), member))

/**
 * navi_clist_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define navi_clist_for_each_entry_safe(pos, n, head, member)			\
	for (pos = navi_clist_entry((head)->next, typeof(*pos), member),	\
		n = navi_clist_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = navi_clist_entry(n->member.next, typeof(*n), member))

/**
 * navi_clist_for_each_entry_safe_continue - continue list iteration safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate over list of given type, continuing after current point,
 * safe against removal of list entry.
 */
#define navi_clist_for_each_entry_safe_continue(pos, n, head, member) 		\
	for (pos = navi_clist_entry(pos->member.next, typeof(*pos), member), 		\
		n = navi_clist_entry(pos->member.next, typeof(*pos), member);		\
	     &pos->member != (head);						\
	     pos = n, n = navi_clist_entry(n->member.next, typeof(*n), member))

/**
 * navi_clist_for_each_entry_safe_from - iterate over list from current point safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate over list of given type from current point, safe against
 * removal of list entry.
 */
#define navi_clist_for_each_entry_safe_from(pos, n, head, member) 			\
	for (n = navi_clist_entry(pos->member.next, typeof(*pos), member);		\
	     &pos->member != (head);						\
	     pos = n, n = navi_clist_entry(n->member.next, typeof(*n), member))

/**
 * navi_clist_for_each_entry_safe_reverse - iterate backwards over list safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate backwards over list of given type, safe against removal
 * of list entry.
 */
#define navi_clist_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = navi_clist_entry((head)->prev, typeof(*pos), member),	\
		n = navi_clist_entry(pos->member.prev, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = navi_clist_entry(n->member.prev, typeof(*n), member))

/**
 * navi_clist_safe_reset_next - reset a stale navi_clist_for_each_entry_safe loop
 * @pos:	the loop cursor used in the navi_clist_for_each_entry_safe loop
 * @n:		temporary storage used in navi_clist_for_each_entry_safe
 * @member:	the name of the list_struct within the struct.
 *
 * navi_clist_safe_reset_next is not safe to use in general if the list may be
 * modified concurrently (eg. the lock is dropped in the loop body). An
 * exception to this is if the cursor element (pos) is pinned in the list,
 * and navi_clist_safe_reset_next is called after re-taking the lock and before
 * completing the current iteration of the loop body.
 */
#define navi_clist_safe_reset_next(pos, n, member)				\
	n = navi_clist_entry(pos->member.next, typeof(*pos), member)

/*
 * Double linked lists with a single pointer list head.
 * Mostly useful for hash tables where the two pointer list head is
 * too wasteful.
 * You lose the ability to access the tail in O(1).
 */

#define NAVI_HLIST_HEAD_INIT { .first = NULL }
#define NAVI_HLIST_HEAD(name) struct navi_hlist_head name = {  .first = NULL }
#define NAVI_INIT_HLIST_HEAD(ptr) ((ptr)->first = NULL)
static inline void NAVI_INIT_HLIST_NODE(struct navi_hlist_node *h)
{
	h->next = NULL;
	h->pprev = NULL;
}

static inline int navi_hlist_unhashed(const struct navi_hlist_node *h)
{
	return !h->pprev;
}

static inline int navi_hlist_empty(const struct navi_hlist_head *h)
{
	return !h->first;
}

static inline void __navi_hlist_del(struct navi_hlist_node *n)
{
	struct navi_hlist_node *next = n->next;
	struct navi_hlist_node **pprev = n->pprev;
	*pprev = next;
	if (next)
		next->pprev = pprev;
}

static inline void navi_hlist_del(struct navi_hlist_node *n)
{
	__navi_hlist_del(n);
	n->next = NAVI_LIST_POISON1;
	n->pprev = NAVI_LIST_POISON2;
}

static inline void navi_hlist_del_init(struct navi_hlist_node *n)
{
	if (!navi_hlist_unhashed(n)) {
		__navi_hlist_del(n);
		NAVI_INIT_HLIST_NODE(n);
	}
}

static inline void navi_hlist_add_head(struct navi_hlist_node *n, struct navi_hlist_head *h)
{
	struct navi_hlist_node *first = h->first;
	n->next = first;
	if (first)
		first->pprev = &n->next;
	h->first = n;
	n->pprev = &h->first;
}

/* next must be != NULL */
static inline void navi_hlist_add_before(struct navi_hlist_node *n,
					struct navi_hlist_node *next)
{
	n->pprev = next->pprev;
	n->next = next;
	next->pprev = &n->next;
	*(n->pprev) = n;
}

static inline void navi_hlist_add_after(struct navi_hlist_node *n,
					struct navi_hlist_node *next)
{
	next->next = n->next;
	n->next = next;
	next->pprev = &n->next;

	if(next->next)
		next->next->pprev  = &next->next;
}

/* after that we'll appear to be on some hlist and navi_hlist_del will work */
static inline void navi_hlist_add_fake(struct navi_hlist_node *n)
{
	n->pprev = &n->next;
}

/*
 * Move a list from one list head to another. Fixup the pprev
 * reference of the first entry if it exists.
 */
static inline void navi_hlist_move_list(struct navi_hlist_head *old,
				   struct navi_hlist_head *new)
{
	new->first = old->first;
	if (new->first)
		new->first->pprev = &new->first;
	old->first = NULL;
}

#define navi_hlist_entry(ptr, type, member) container_of(ptr,type,member)

#define navi_hlist_for_each(pos, head) \
	for (pos = (head)->first; pos ; pos = pos->next)

#define navi_hlist_for_each_safe(pos, n, head) \
	for (pos = (head)->first; pos && ({ n = pos->next; 1; }); \
	     pos = n)

#define navi_hlist_entry_safe(ptr, type, member) \
	({ typeof(ptr) ____ptr = (ptr); \
	   ____ptr ? navi_hlist_entry(____ptr, type, member) : NULL; \
	})

/**
 * navi_hlist_for_each_entry	- iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the hlist_node within the struct.
 */
#define navi_hlist_for_each_entry(pos, head, member)				\
	for (pos = navi_hlist_entry_safe((head)->first, typeof(*(pos)), member);\
	     pos;							\
	     pos = navi_hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

/**
 * navi_hlist_for_each_entry_continue - iterate over a hlist continuing after current point
 * @pos:	the type * to use as a loop cursor.
 * @member:	the name of the hlist_node within the struct.
 */
#define navi_hlist_for_each_entry_continue(pos, member)			\
	for (pos = navi_hlist_entry_safe((pos)->member.next, typeof(*(pos)), member);\
	     pos;							\
	     pos = navi_hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

/**
 * navi_hlist_for_each_entry_from - iterate over a hlist continuing from current point
 * @pos:	the type * to use as a loop cursor.
 * @member:	the name of the hlist_node within the struct.
 */
#define navi_hlist_for_each_entry_from(pos, member)				\
	for (; pos;							\
	     pos = navi_hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

/**
 * navi_hlist_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another &struct hlist_node to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the hlist_node within the struct.
 */
#define navi_hlist_for_each_entry_safe(pos, n, head, member) 		\
	for (pos = navi_hlist_entry_safe((head)->first, typeof(*pos), member);\
	     pos && ({ n = pos->member.next; 1; });			\
	     pos = navi_hlist_entry_safe(n, typeof(*pos), member))

#endif
