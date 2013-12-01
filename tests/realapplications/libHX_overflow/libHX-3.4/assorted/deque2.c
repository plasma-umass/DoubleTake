/*
 *	libHX/assorted/deque2.c
 *	Copyright © Jan Engelhardt <jengelh [at] medozas de>, 2002 - 2007
 *
 *	This file is part of libHX. libHX is free software; you can
 *	redistribute it and/or modify it under the terms of the GNU
 *	Lesser General Public License as published by the Free Software
 *	Foundation; either version 2 or 3 of the License.
 *
 *	deque2.c:
 *	Assorted DEQUE functions that are not deemed to be useful in the
 *	(compiled) library at this time.
 */
#include <stdio.h>
#include <libHX.h>

EXPORT_SYMBOL struct HXdeque_node *HXdeque_rfind(struct HXdeque *dq,
    const void *ptr)
{
	struct HXdeque_node *trav;
	for (trav = dq->last; trav != NULL; trav = trav->prev)
		if (trav->ptr == ptr)
			return trav;
	return NULL;
}

EXPORT_SYMBOL void *HXdeque_rget(struct HXdeque *dq, const void *ptr)
{
	struct HXdeque_node *trav;
	for (trav = dq->last; trav != NULL; trav = trav->prev)
		if (trav->ptr == ptr)
			return trav->ptr;
	return NULL;
}

EXPORT_SYMBOL void *HXdeque_sget(struct HXdeque *dq, const char *s)
{
	struct HXdeque_node *trav;
	for (trav = dq->first; trav != NULL; trav = trav->next)
		if (strcmp(trav->ptr, s) == 0)
			return trav->ptr;
	return NULL;
}

EXPORT_SYMBOL struct HXdeque_node *HXdeque_dup(struct HXdeque *dq)
{
	/*
	 * Duplicate the object on top of the stack by popping it off and
	 * adding it again, twice.
	 */
	if (dq->last == NULL)
		return NULL;

	/*
	 * The mathematical axiomatic definition is that the last element is
	 * popped off and pushed twice. We optimize by simply "looking" at the
	 * last and push it again.
	 */
	return HXdeque_push(dq, dq->last->ptr);
}

EXPORT_SYMBOL struct HXdeque_node *HXdeque_rdup(struct HXdeque *dq)
{
	/* Same as HXdeque_dup(), but works on the bottom of the stack */
	if (dq->first == NULL)
		return NULL;
	return HXdeque_unshift(dq, dq->first->ptr);
}

EXPORT_SYMBOL struct HXdeque_node *HXdeque_toprr(struct HXdeque *dq)
{
	/*
	 * Rotates the topmost three items right ([bottom]...CBA[top] =>
	 * [bottom]...ACB[top]). Also works if there are only two items in the
	 * stack.
	 */
	struct HXdeque_node *p = dq->last;
	if (p == NULL)
		return NULL;
	HXdeque_down(p);
	HXdeque_down(p);
	return p;
}

EXPORT_SYMBOL struct HXdeque_node *HXdeque_toprl(struct HXdeque *dq)
{
	/* Rotates the topmost three items left (...CBA => ...BAC) */
	struct HXdeque_node *p = dq->last;
	if (p == NULL)
		return NULL;
	if (p->Prev != NULL) p = p->Prev;
	if (p->Prev != NULL) p = p->Prev;
	HXdeque_up(p);
	HXdeque_up(p);
	return p;
}

EXPORT_SYMBOL struct HXdeque_node *HXdeque_botrr(struct HXdeque *dq)
{
	/* (CBA... => ...ACB) */
	struct HXdeque_node *p = dq->first;
	if (p == NULL)
		return NULL;
	if (p->Prev != NULL) p = p->Prev;
	if (p->Prev != NULL) p = p->Prev;
	HXdeque_down(p);
	HXdeque_down(p);
	return p;
}

EXPORT_SYMBOL struct HXdeque_node *HXdeque_botrl(struct HXdeque *dq)
{
	struct HXdeque_node *p = dq->first;
	if (p == NULL)
		return NULL;
	HXdeque_up(p);
	HXdeque_up(p);
	return p;
}
