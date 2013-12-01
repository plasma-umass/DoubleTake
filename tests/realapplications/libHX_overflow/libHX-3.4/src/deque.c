/*
 *	Double-ended queues
 *	Copyright © Jan Engelhardt <jengelh [at] medozas de>, 2002 - 2008
 *
 *	This file is part of libHX. libHX is free software; you can
 *	redistribute it and/or modify it under the terms of the GNU
 *	Lesser General Public License as published by the Free Software
 *	Foundation; either version 2.1 or 3 of the License.
 */
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <libHX/deque.h>
#include <libHX/string.h>
#include "internal.h"

static inline void HXdeque_add(struct HXdeque_node *af,
    struct HXdeque_node *nd)
{
	struct HXdeque *parent = af->parent;
	nd->next   = af->next;
	nd->prev   = af;
	af->next   = nd;
	nd->parent = parent;
	if (parent->last == af)
		parent->last = nd;
}

static inline void HXdeque_drop(struct HXdeque *parent,
    struct HXdeque_node *node)
{
	struct HXdeque_node *left = node->prev, *right = node->next;

	if (left == NULL) parent->first = right;
	else              left->next = right;

	if (right == NULL) parent->last = left;
	else               right->prev = left;
}

EXPORT_SYMBOL struct HXdeque *HXdeque_init(void)
{
	struct HXdeque *dq;
	if ((dq = calloc(1, sizeof(struct HXdeque))) == NULL)
		return NULL;
	return dq;
}

EXPORT_SYMBOL struct HXdeque_node *HXdeque_push(struct HXdeque *dq,
    const void *ptr)
{
	struct HXdeque_node *nd;
	if ((nd = malloc(sizeof(struct HXdeque_node))) == NULL)
		return NULL;
	nd->prev   = dq->last;
	nd->next   = NULL;
	nd->parent = dq;
	nd->ptr    = const_cast1(void *, ptr);

	if (dq->first == NULL) {
		dq->first = dq->last = nd;
	} else {
		dq->last->next = nd;
		dq->last = nd;
	}

	++dq->items;
	return nd;
}

EXPORT_SYMBOL void *HXdeque_pop(struct HXdeque *dq)
{
	if (dq->last == NULL)
		return NULL;
	return HXdeque_del(dq->last);
}

EXPORT_SYMBOL struct HXdeque_node *HXdeque_unshift(struct HXdeque *dq,
    const void *ptr)
{
	struct HXdeque_node *nd;

	if (dq->first == NULL)
		return HXdeque_push(dq, ptr);
	if ((nd = malloc(sizeof(struct HXdeque_node))) == NULL)
		return NULL;

	nd->prev   = NULL;
	nd->next   = dq->first;
	nd->parent = dq;
	nd->ptr    = const_cast1(void *, ptr);

	dq->first->prev = nd;
	dq->first = nd;
	++dq->items;
	return nd;
}

EXPORT_SYMBOL void *HXdeque_shift(struct HXdeque *dq)
{
	if (dq->first == NULL)
		return NULL;
	return HXdeque_del(dq->first);
}

EXPORT_SYMBOL void HXdeque_move(struct HXdeque_node *nd,
    struct HXdeque_node *af)
{
	HXdeque_drop(nd->parent, nd);
	HXdeque_add(af, nd);
}

EXPORT_SYMBOL void *HXdeque_del(struct HXdeque_node *node)
{
	void *ret = node->ptr;
	HXdeque_drop(node->parent, node);
	--node->parent->items;
	free(node);
	return ret;
}

EXPORT_SYMBOL void HXdeque_free(struct HXdeque *dq)
{
	struct HXdeque_node *node, *next;
	for (node = dq->first; node != NULL; node = next) {
		next = node->next;
		free(node);
	}
	free(dq);
}

EXPORT_SYMBOL struct HXdeque_node *HXdeque_find(struct HXdeque *dq,
  const void *ptr)
{
	struct HXdeque_node *travp;
	for (travp = dq->first; travp != NULL; travp = travp->next)
		if (travp->ptr == ptr)
			return travp;
	return NULL;
}

EXPORT_SYMBOL void *HXdeque_get(struct HXdeque *dq, const void *ptr)
{
	struct HXdeque_node *trav = dq->first;
	for (trav = dq->first; trav != NULL; trav = trav->next)
		if (trav->ptr == ptr)
			return trav->ptr;
	return NULL;
}

EXPORT_SYMBOL void HXdeque_genocide2(struct HXdeque *dq, void (*xfree)(void *))
{
	struct HXdeque_node *trav, *next;
	for (trav = dq->first; trav != NULL; trav = next) {
		next = trav->next;
		xfree(trav->ptr);
		free(trav);
	}
	free(dq);
}

EXPORT_SYMBOL void **HXdeque_to_vec(const struct HXdeque *dq, unsigned int *num)
{
	const struct HXdeque_node *trav;
	void **ret, **p;

	ret = malloc((dq->items + 1) * sizeof(void *));
	if (ret == NULL)
		return NULL;

	p = ret;
	for (trav = dq->first; trav != NULL; trav = trav->next)
		*p++ = trav->ptr;
	*p = NULL;

	if (num != NULL)
		*num = dq->items;
	return ret;
}
