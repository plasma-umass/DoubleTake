/*
 *	Maps (key-value pairs)
 *	Copyright © Jan Engelhardt <jengelh [at] medozas de>, 2009
 *
 *	This file is part of libHX. libHX is free software; you can
 *	redistribute it and/or modify it under the terms of the GNU
 *	Lesser General Public License as published by the Free Software
 *	Foundation; either version 2.1 or 3 of the License.
 *
 *	Incorporates Public Domain code from Bob Jenkins's lookup3 (May 2006)
 */
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libHX/list.h>
#include <libHX/map.h>
#include <libHX/string.h>
#include "internal.h"
#include "map_int.h"

/*
 * http://planetmath.org/encyclopedia/GoodHashTablePrimes.html
 * 23 and 3221.. added by j.eng.
 */
EXPORT_SYMBOL const unsigned int HXhash_primes[] = {
	23, 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157,
	98317, 196613, 393241, 786433, 1572869, 3145739, 6291469, 12582917,
	25165843, 50331653, 100663319, 201326611, 402653189, 805306457,
	1610612741, 3221225473U,
};

static void HXhmap_free(struct HXhmap *hmap)
{
	struct HXhmap_node *drop, *dnext;
	unsigned int i;

	for (i = 0; i < HXhash_primes[hmap->power]; ++i) {
		HXlist_for_each_entry_safe(drop, dnext,
		    &hmap->bk_array[i], anchor) {
			if (hmap->super.ops.k_free != NULL)
				hmap->super.ops.k_free(drop->key);
			if (hmap->super.ops.d_free != NULL)
				hmap->super.ops.d_free(drop->data);
			free(drop);
		}
	}

	free(hmap->bk_array);
	free(hmap);
}

static void HXrbtree_free_dive(const struct HXrbtree *btree,
    struct HXrbtree_node *node)
{
	/*
	 * Recursively dives into the tree and destroys elements. Note that you
	 * shall use this when destroying a complete tree instead of iterated
	 * deletion with HXrbtree_del(). Since this functions is meant to free
	 * it all, it does not need to care about rebalancing.
	 */
	if (node->sub[RBT_LEFT] != NULL)
		HXrbtree_free_dive(btree, node->sub[RBT_LEFT]);
	if (node->sub[RBT_RIGHT] != NULL)
		HXrbtree_free_dive(btree, node->sub[RBT_RIGHT]);
	if (btree->super.ops.k_free != NULL)
		btree->super.ops.k_free(node->key);
	if (btree->super.ops.d_free != NULL)
		btree->super.ops.d_free(node->data);
	free(node);
}

static void HXrbtree_free(struct HXrbtree *btree)
{
	if (btree->root != NULL)
		HXrbtree_free_dive(btree, btree->root);
	free(btree);
}

EXPORT_SYMBOL void HXmap_free(struct HXmap *xmap)
{
	void *vmap = xmap;
	const struct HXmap_private *map = vmap;

	switch (map->type) {
	case HXMAPT_HASH:
		return HXhmap_free(vmap);
	case HXMAPT_RBTREE:
		return HXrbtree_free(vmap);
	default:
		break;
	}
}

static int HXmap_valuecmp(const void *pa, const void *pb, size_t len)
{
	/*
	 * Cannot use "pa - pb" as that could underflow. 
	 * Also, while "return (pa > pb) - (pa < pb)" does not use a branch,
	 * it compiles to more instructions and seems to be slower on x86.
	 */
	return (pa > pb) ? 1 : (pa < pb) ? -1 : 0;
}

static void *HXmap_valuecpy(const void *p, size_t len)
{
	return const_cast1(void *, p);
}

#define jrot(x,k) (((x) << (k)) | ((x) >> (32 - (k))))

/* jhash_mix - mix 3 32-bit values reversibly. */
#define jhash_mix(a, b, c) { \
	a -= c; a ^= jrot(c,  4); c += b; \
	b -= a; b ^= jrot(a,  6); a += c; \
	c -= b; c ^= jrot(b,  8); b += a; \
	a -= c; a ^= jrot(c, 16); c += b; \
	b -= a; b ^= jrot(a, 19); a += c; \
	c -= b; c ^= jrot(b,  4); b += a; \
}

#define jhash_final(a, b, c) { \
	c ^= b; c -= jrot(b, 14); \
	a ^= c; a -= jrot(c, 11); \
	b ^= a; b -= jrot(a, 25); \
	c ^= b; c -= jrot(b, 16); \
	a ^= c; a -= jrot(c,  4);  \
	b ^= a; b -= jrot(a, 14); \
	c ^= b; c -= jrot(b, 24); \
}

EXPORT_SYMBOL unsigned long HXhash_jlookup3(const void *vkey, size_t length)
{
	static const unsigned int JHASH_GOLDEN_RATIO = 0x9e3779b9;
	const uint8_t *key = vkey;
	uint32_t a, b, c;

	a = b = c = JHASH_GOLDEN_RATIO + length;
	/* All but the last block: affect some 32 bits of (a,b,c) */
	for (; length > 12; length -= 12, key += 12) {
		a += key[0] + ((uint32_t)key[1] << 8) +
		     ((uint32_t)key[2] << 16) + ((uint32_t)key[3] << 24);
		b += key[4] + ((uint32_t)key[5] << 8) +
		     ((uint32_t)key[6] << 16) + ((uint32_t)key[7] << 24);
		c += key[8] + ((uint32_t)key[9] << 8) +
		     ((uint32_t)key[10] << 16)+ ((uint32_t)key[11] << 24);
		jhash_mix(a, b, c);
	}

	switch (length) {
	case 12: c += ((uint32_t)key[11]) << 24;
	case 11: c += ((uint32_t)key[10]) << 16;
	case 10: c += ((uint32_t)key[9])  << 8;
	case  9: c += key[8];
	case  8: b += ((uint32_t)key[7]) << 24;
	case  7: b += ((uint32_t)key[6]) << 16;
	case  6: b += ((uint32_t)key[5]) << 8;
	case  5: b += key[4];
	case  4: a += ((uint32_t)key[3]) << 24;
	case  3: a += ((uint32_t)key[2]) << 16;
	case  2: a += ((uint32_t)key[1]) << 8;
	case  1: a += key[0];
		break;
	case  0: return c;
	}

	jhash_final(a,b,c);
	return c;
}

static unsigned long HXhash_jlookup3v(const void *p, size_t z)
{
	return HXhash_jlookup3(&p, sizeof(p));
}

EXPORT_SYMBOL unsigned long HXhash_jlookup3s(const void *p, size_t z)
{
	return HXhash_jlookup3(p, strlen(p));
}

EXPORT_SYMBOL unsigned long HXhash_djb2(const void *p, size_t z)
{
	const char *c = p;
	unsigned long v = 5381;

	while (*c != '\0')
		v = ((v << 5) + v) ^ *c++;
		/* v = v * 33 ^ *c++; */

	return v;
}

/**
 * Set up the operations for a map based on flags, and then override with
 * user-specified functions.
 */
static void HXmap_ops_setup(struct HXmap_private *super,
    const struct HXmap_ops *new_ops)
{
	struct HXmap_ops *ops = &super->ops;

	ops->k_clone = HXmap_valuecpy;
	ops->d_clone = HXmap_valuecpy;

	if (super->flags & HXMAP_SKEY)
		ops->k_compare = static_cast(void *, strcmp);
	else if (super->key_size == 0)
		ops->k_compare = HXmap_valuecmp;
	else
		ops->k_compare = memcmp;

	if (super->flags & HXMAP_CKEY) {
		ops->k_clone = (super->flags & HXMAP_SKEY) ?
		               static_cast(void *, HX_strdup) : HX_memdup;
		ops->k_free  = free;
	}
	if (super->flags & HXMAP_CDATA) {
		ops->d_clone = (super->flags & HXMAP_SDATA) ?
		               static_cast(void *, HX_strdup) : HX_memdup;
		ops->d_free  = free;
	}

	if (super->type == HXMAPT_HASH) {
		if (super->flags & HXMAP_SKEY)
			ops->k_hash = HXhash_djb2;
		else if (super->key_size != 0)
			ops->k_hash = HXhash_jlookup3;
		else
			ops->k_hash = HXhash_jlookup3v;
	}

	if (new_ops == NULL)
		return;

	/* Update with user-supplied functions */
	if (new_ops->k_compare != NULL)
		ops->k_compare = new_ops->k_compare;
	if (new_ops->k_clone != NULL)
		ops->k_clone   = new_ops->k_clone;
	if (new_ops->k_free != NULL)
		ops->k_free    = new_ops->k_free;
	if (new_ops->d_clone != NULL)
		ops->d_clone   = new_ops->d_clone;
	if (new_ops->d_free != NULL)
		ops->d_free    = new_ops->d_free;
	if (super->type == HXMAPT_HASH && new_ops->k_hash != NULL)
		ops->k_hash    = new_ops->k_hash;
}

/**
 * @n:	nominator of fraction
 * @d:	denominator of fraction
 * @v:	value
 *
 * Calculates @v * (@n / @d) without floating point or risk of overflow
 * (when @n <= @d).
 */
static inline unsigned int x_frac(unsigned int n, unsigned int d,
    unsigned int v)
{
	return (v / d) * n + (v % d) * n / d;
}

/**
 * HXhmap_move - move elements from one map to another
 * @bk_array:	target bucket array
 * @bk_number:	number of buckets
 * @hmap:	old hash table
 */
static void HXhmap_move(struct HXlist_head *bk_array, unsigned int bk_number,
    struct HXhmap *hmap)
{
	struct HXhmap_node *drop, *dnext;
	unsigned int bk_idx, i;

	for (i = 0; i < HXhash_primes[hmap->power]; ++i)
		HXlist_for_each_entry_safe(drop, dnext,
		    &hmap->bk_array[i], anchor) {
			bk_idx = hmap->super.ops.k_hash(drop->key,
			         hmap->super.key_size) % bk_number;
			HXlist_del(&drop->anchor);
			HXlist_add_tail(&bk_array[bk_idx], &drop->anchor);
		}
}

/**
 * HXhmap_layout - resize and rehash table
 * @hmap:	hash map
 * @prime_idx:	requested new table size (prime power thereof)
 */
static int HXhmap_layout(struct HXhmap *hmap, unsigned int power)
{
	const unsigned int bk_number = HXhash_primes[power];
	struct HXlist_head *bk_array, *old_array = NULL;
	unsigned int i;

	bk_array = malloc(bk_number * sizeof(*bk_array));
	if (bk_array == NULL)
		return -errno;
	for (i = 0; i < bk_number; ++i)
		HXlist_init(&bk_array[i]);
	if (hmap->bk_array != NULL) {
		HXhmap_move(bk_array, bk_number, hmap);
		old_array = hmap->bk_array;
	}
	hmap->power    = power;
	hmap->min_load = (power != 0) ? HXhash_primes[power] / 4 : 0;
	hmap->max_load = x_frac(7, 10, HXhash_primes[power]);
	hmap->bk_array = bk_array;
	++hmap->tid;
	free(old_array);
	return 1;
}

static struct HXmap *HXhashmap_init4(unsigned int flags,
    const struct HXmap_ops *ops, size_t key_size, size_t data_size)
{
	struct HXmap_private *super;
	struct HXhmap *hmap;
	int saved_errno;

	if ((hmap = calloc(1, sizeof(*hmap))) == NULL)
		return NULL;

	super            = &hmap->super;
	super->flags     = flags;
	super->items     = 0;
	super->type      = HXMAPT_HASH;
	super->key_size  = key_size;
	super->data_size = data_size;
	HXmap_ops_setup(super, ops);
	hmap->tid = 1;
	errno = HXhmap_layout(hmap, 0);
	if (hmap->bk_array == NULL)
		goto out;

	errno = 0;
	return static_cast(void *, hmap);

 out:
	saved_errno = errno;
	HXhmap_free(hmap);
	errno = saved_errno;
	return NULL;
}

static struct HXmap *HXrbtree_init4(unsigned int flags,
    const struct HXmap_ops *ops, size_t key_size, size_t data_size)
{
	struct HXmap_private *super;
	struct HXrbtree *btree;

	BUILD_BUG_ON(offsetof(struct HXrbtree, root) +
	             offsetof(struct HXrbtree_node, sub[0]) !=
	             offsetof(struct HXrbtree, root));

	if ((btree = calloc(1, sizeof(*btree))) == NULL)
		return NULL;

	super            = &btree->super;
	super->type      = HXMAPT_RBTREE;
	super->flags     = flags;
	super->items     = 0;
	super->key_size  = key_size;
	super->data_size = data_size;
	HXmap_ops_setup(super, ops);

	/*
	 * TID must not be zero, otherwise the traverser functions will not
	 * start off correctly, since trav->tid is 0, but trav->tid must not
	 * equal btree->transact because that would mean the traverser is in
	 * sync with the tree.
	 */
	btree->tid  = 1;
	btree->root = NULL;
	return static_cast(void *, btree);
}

EXPORT_SYMBOL struct HXmap *HXmap_init5(enum HXmap_type type,
    unsigned int flags, const struct HXmap_ops *ops, size_t key_size,
    size_t data_size)
{
	switch (type) {
	case HXMAPT_HASH:
		return HXhashmap_init4(flags, ops, key_size, data_size);
	case HXMAPT_RBTREE:
		return HXrbtree_init4(flags, ops, key_size, data_size);
	default:
		errno = -ENOENT;
		return NULL;
	}
}

EXPORT_SYMBOL struct HXmap *HXmap_init(enum HXmap_type type,
    unsigned int flags)
{
	return HXmap_init5(type, flags, NULL, 0, 0);
}

static struct HXhmap_node *HXhmap_find(const struct HXhmap *hmap,
    const void *key)
{
	struct HXhmap_node *drop;
	unsigned int bk_idx;

	bk_idx = hmap->super.ops.k_hash(key, hmap->super.key_size) %
	         HXhash_primes[hmap->power];
	HXlist_for_each_entry(drop, &hmap->bk_array[bk_idx], anchor)
		if (hmap->super.ops.k_compare(key, drop->key,
		    hmap->super.key_size) == 0)
			return drop;
	return NULL;
}

static const struct HXmap_node *HXrbtree_find(const struct HXrbtree *btree,
    const void *key)
{
	struct HXrbtree_node *node = btree->root;
	int res;

	while (node != NULL) {
		if ((res = btree->super.ops.k_compare(key,
		    node->key, btree->super.key_size)) == 0)
			return static_cast(const void *, &node->key);
		node = node->sub[res > 0];
	}

	return NULL;
}

EXPORT_SYMBOL const struct HXmap_node *
HXmap_find(const struct HXmap *xmap, const void *key)
{
	const void *vmap = xmap;
	const struct HXmap_private *map = vmap;

	switch (map->type) {
	case HXMAPT_HASH: {
		const struct HXhmap_node *node = HXhmap_find(vmap, key);
		if (node == NULL)
			return NULL;
		return static_cast(const void *, &node->key);
	}
	case HXMAPT_RBTREE:
		return HXrbtree_find(vmap, key);
	default:
		errno = EINVAL;
		return NULL;
	}
}

EXPORT_SYMBOL void *HXmap_get(const struct HXmap *map, const void *key)
{
	const struct HXmap_node *node;

	if ((node = HXmap_find(map, key)) == NULL) {
		errno = ENOENT;
		return NULL;
	}
	errno = 0;
	return node->data;
}

/**
 * HXhmap_replace - replace value in a drop
 */
static int HXhmap_replace(const struct HXhmap *hmap, struct HXhmap_node *drop,
    const void *value)
{
	void *old_value, *new_value;

	if (hmap->super.flags & HXMAP_NOREPLACE)
		return -EEXIST;

	new_value = hmap->super.ops.d_clone(value, hmap->super.data_size);
	if (new_value == NULL && value != NULL)
		return -errno;
	old_value  = drop->data;
	drop->data = new_value;
	if (hmap->super.ops.d_free != NULL)
		hmap->super.ops.d_free(old_value);
	return 1;
}

static int HXhmap_add(struct HXhmap *hmap, const void *key, const void *value)
{
	struct HXhmap_node *drop;
	unsigned int bk_idx;
	int ret, saved_errno;

	if ((drop = HXhmap_find(hmap, key)) != NULL)
		return HXhmap_replace(hmap, drop, value);

	if (hmap->super.items >= hmap->max_load &&
	    hmap->power < ARRAY_SIZE(HXhash_primes) - 1) {
		if ((ret = HXhmap_layout(hmap, hmap->power + 1)) <= 0)
			return ret;
	} else if (hmap->super.items < hmap->min_load && hmap->power > 0) {
		if ((ret = HXhmap_layout(hmap, hmap->power - 1)) <= 0)
			return ret;
	}

	/* New node */
	if ((drop = malloc(sizeof(*drop))) == NULL)
		return -errno;
	HXlist_init(&drop->anchor);
	drop->key = hmap->super.ops.k_clone(key, hmap->super.key_size);
	if (drop->key == NULL && key != NULL)
		goto out;
	drop->data = hmap->super.ops.d_clone(value, hmap->super.data_size);
	if (drop->data == NULL && value != NULL)
		goto out;

	bk_idx = hmap->super.ops.k_hash(key, hmap->super.key_size) %
	         HXhash_primes[hmap->power];
	HXlist_add_tail(&hmap->bk_array[bk_idx], &drop->anchor);
	++hmap->super.items;
	return 1;

 out:
	saved_errno = errno;
	if (hmap->super.ops.k_free != NULL)
		hmap->super.ops.k_free(drop->key);
	free(drop);
	return -(errno = saved_errno);
}

/**
 * HXrbtree_amov - do balance (move) after addition of a node
 * @path:	path from the root to the new node
 * @dir:	direction vectors
 * @depth:	current index in @path and @dir
 * @tid:	pointer to transaction ID which may need updating
 */
static void HXrbtree_amov(struct HXrbtree_node **path,
    const unsigned char *dir, unsigned int depth, unsigned int *tid)
{
	struct HXrbtree_node *uncle, *parent, *grandp, *newnode;

	/*
	 * The newly inserted node (or the last rebalanced node) at
	 * @path[depth-1] is red, so the parent must not be.
	 *
	 * Use an iterative approach to not waste time with recursive function
	 * calls. The @LR variable is used to handle the symmetric case without
	 * code duplication.
	 */
	do {
		unsigned int LR = dir[depth-2];

		grandp = path[depth-2];
		parent = path[depth-1];
		uncle  = grandp->sub[!LR];

		if (uncle != NULL && uncle->color == RBT_RED) {
			/*
			 * Case 3 (WP): Only colors have to be swapped to keep
			 * the black height. But rebalance needs to continue.
			 */
			parent->color = RBT_BLACK;
			uncle->color  = RBT_BLACK;
			grandp->color = RBT_RED;
			depth        -= 2;
			continue;
		}

		/*
		 * Case 4 (WP): New node is the right child of its parent, and
		 * the parent is the left child of the grandparent. A left
		 * rotate is done at the parent to transform it into a case 5.
		 */
		if (dir[depth-1] != LR) {
			newnode          = parent->sub[!LR];
			parent->sub[!LR] = newnode->sub[LR];
			newnode->sub[LR] = parent;
			grandp->sub[LR]  = newnode;
			/* relabel */
			parent  = grandp->sub[LR];
			newnode = parent->sub[LR];
		} else {
			newnode = path[depth];
		}

		/*
		 * Case 5: New node is the @LR child of its parent which is
		 * the @LR child of the grandparent. A right rotation on
		 * @grandp is performed.
		 */
		grandp->sub[LR]  = parent->sub[!LR];
		parent->sub[!LR] = grandp;
		path[depth-3]->sub[dir[depth-3]] = parent;
		grandp->color    = RBT_RED;
		parent->color    = RBT_BLACK;
		++*tid;
		break;
	} while (depth >= 3 && path[depth-1]->color == RBT_RED);
}

static int HXrbtree_replace(const struct HXrbtree *btree,
    struct HXrbtree_node *node, const void *value)
{
	void *old_value, *new_value;

	if (!(btree->super.flags & HXMAP_NOREPLACE))
		return -(errno = EEXIST);

	new_value = btree->super.ops.d_clone(value, btree->super.data_size);
	if (new_value == NULL && value != NULL)
		return -errno;
	old_value  = node->data;
	node->data = new_value;
	if (btree->super.ops.d_free != NULL)
		btree->super.ops.d_free(old_value);
	return 1;
}

static int HXrbtree_add(struct HXrbtree *btree,
    const void *key, const void *value)
{
	struct HXrbtree_node *node, *path[RBT_MAXDEP];
	unsigned char dir[RBT_MAXDEP];
	unsigned int depth = 0;
	int saved_errno;

	/*
	 * Since our struct HXrbtree_node runs without a ->parent pointer,
	 * the path "upwards" from @node needs to be recorded somehow,
	 * here with @path. Another array, @dir is used to speedup direction
	 * decisions. (WP's "n->parent == grandparent(n)->left" is just slow.)
	 */
	path[depth]  = reinterpret_cast(struct HXrbtree_node *, &btree->root);
	dir[depth++] = 0;
	node = btree->root;

	while (node != NULL) {
		int res = btree->super.ops.k_compare(key,
		          node->key, btree->super.key_size);
		if (res == 0)
			/*
			 * The node already exists (found the key), overwrite
			 * the data.
			 */
			return HXrbtree_replace(btree, node, value);

		res          = res > 0;
		path[depth]  = node;
		dir[depth++] = res;
		node         = node->sub[res];
	}

	if ((node = malloc(sizeof(struct HXrbtree_node))) == NULL)
		return -errno;

	/* New node, push data into it */
	node->key = btree->super.ops.k_clone(key, btree->super.key_size);
	if (node->key == NULL && key != NULL)
		goto out;
	node->data = btree->super.ops.d_clone(value, btree->super.data_size);
	if (node->data == NULL && value != NULL)
		goto out;

	/*
	 * Add the node to the tree. In trying not to hit a rule 2 violation
	 * (each simple path has the same number of black nodes), it is colored
	 * red so that below we only need to check for rule 1 violations.
	 */
	node->sub[RBT_LEFT] = node->sub[RBT_RIGHT] = NULL;
	node->color = RBT_RED;
	path[depth-1]->sub[dir[depth-1]] = node;
	++btree->super.items;

	/*
	 * WP: [[Red-black_tree]] says:
	 * Case 1: @node is root node - just color it black (see below).
	 * Case 2: @parent is black - no action needed (skip).
	 * No rebalance needed for a 2-node tree.
	 */
	if (depth >= 3 && path[depth-1]->color == RBT_RED)
		HXrbtree_amov(path, dir, depth, &btree->tid);

	btree->root->color = RBT_BLACK;
	return 1;

 out:
	saved_errno = errno;
	if (btree->super.ops.k_free != NULL)
		btree->super.ops.k_free(node->key);
	if (btree->super.ops.d_free != NULL)
		btree->super.ops.d_free(node->key);
	free(node);
	return -(errno = saved_errno);
}

EXPORT_SYMBOL int HXmap_add(struct HXmap *xmap,
    const void *key, const void *value)
{
	void *vmap = xmap;
	struct HXmap_private *map = vmap;

	if ((map->flags & HXMAP_SINGULAR) && value != NULL) {
		fprintf(stderr, "libHX-map: adding value!=NULL "
		        "into a set not allowed\n");
		return -EINVAL;
	}

	switch (map->type) {
	case HXMAPT_HASH:
		return HXhmap_add(vmap, key, value);
	case HXMAPT_RBTREE:
		return HXrbtree_add(vmap, key, value);
	default:
		return -EINVAL;
	}
}

static void *HXhmap_del(struct HXhmap *hmap, const void *key)
{
	struct HXhmap_node *drop;
	void *value;

	if ((drop = HXhmap_find(hmap, key)) == NULL) {
		errno = ENOENT;
		return NULL;
	}

	HXlist_del(&drop->anchor);
	++hmap->tid;
	--hmap->super.items;
	if (hmap->super.items < hmap->min_load && hmap->power > 0)
		/*
		 * Ignore return value. If it failed, it will continue to use
		 * the current bk_array.
		 */
		HXhmap_layout(hmap, hmap->power - 1);

	value = drop->data;
	if (hmap->super.ops.k_free != NULL)
		hmap->super.ops.k_free(drop->key);
	if (hmap->super.ops.d_free != NULL)
		hmap->super.ops.d_free(drop->data);
	free(drop);
	errno = 0;
	return value;
}

static unsigned int HXrbtree_del_mm(struct HXrbtree_node **path,
    unsigned char *dir, unsigned int depth)
{
	/* Both subtrees exist */
	struct HXrbtree_node *io_node, *io_parent, *orig_node = path[depth];
	unsigned char color;
	unsigned int spos;

	io_node    = orig_node->sub[RBT_RIGHT];
	dir[depth] = RBT_RIGHT;

	if (io_node->sub[RBT_LEFT] == NULL) {
		/* Right subtree node is direct inorder */
		io_node->sub[RBT_LEFT] = orig_node->sub[RBT_LEFT];
		color                = io_node->color;
		io_node->color       = orig_node->color;
		orig_node->color     = color;

		path[depth-1]->sub[dir[depth-1]] = io_node;
		path[depth++]        = io_node;
		return depth;
	}

	/*
	 * Walk down to the leftmost element, keep track of inorder node
	 * and its parent.
	 */
	spos = depth++;

	do {
		io_parent    = io_node;
		path[depth]  = io_parent;
		dir[depth++] = RBT_LEFT;
		io_node      = io_parent->sub[RBT_LEFT];
	} while (io_node->sub[RBT_LEFT] != NULL);

	/* move node up */
	path[spos-1]->sub[dir[spos-1]] = path[spos] = io_node;
	io_parent->sub[RBT_LEFT]         = io_node->sub[RBT_RIGHT];
	io_node->sub[RBT_LEFT]           = orig_node->sub[RBT_LEFT];
	io_node->sub[RBT_RIGHT]          = orig_node->sub[RBT_RIGHT];

	color          = io_node->color;
	io_node->color = orig_node->color;

	/*
	 * The nodes (@io_node and @orig_node) have been swapped. While
	 * @orig_node has no pointers to it, it still exists and decisions are
	 * made upon its properties in HXrbtree_del() and btree_dmov() until it
	 * is freed later. Hence we need to keep the color.
	 */
	orig_node->color = color;
	return depth;
}

static void HXrbtree_dmov(struct HXrbtree_node **path, unsigned char *dir,
    unsigned int depth)
{
	struct HXrbtree_node *w, *x;

	while (true) {
		unsigned char LR = dir[depth - 1];
		x = path[depth - 1]->sub[LR];

		if (x != NULL && x->color == RBT_RED) {
			/* (WP) "delete_one_child" */
			x->color = RBT_BLACK;
			break;
		}

		if (depth < 2)
			/* Case 1 */
			break;

		/* @w is the sibling of @x (the current node). */
		w = path[depth - 1]->sub[!LR];
		if (w->color == RBT_RED) {
			/*
			 * Case 2. @w is of color red. In order to collapse
			 * cases, a left rotate is performed at @x's parent and
			 * colors are swapped to make @w a black node.
			 */
			w->color = RBT_BLACK;
			path[depth - 1]->color = RBT_RED;
			path[depth - 1]->sub[!LR] = w->sub[LR];
			w->sub[LR] = path[depth - 1];
			path[depth - 2]->sub[dir[depth - 2]] = w;
			path[depth] = path[depth - 1];
			dir[depth]  = LR;
			path[depth - 1] = w;
			w = path[++depth - 1]->sub[!LR];
		}

		if ((w->sub[LR] == NULL || w->sub[LR]->color == RBT_BLACK) &&
		   (w->sub[!LR] == NULL || w->sub[!LR]->color == RBT_BLACK)) {
			/* Case 3/4: @w has no red children. */
			w->color = RBT_RED;
			--depth;
			continue;
		}

		if (w->sub[!LR] == NULL || w->sub[!LR]->color == RBT_BLACK) {
			/* Case 5 */
			struct HXrbtree_node *y = w->sub[LR];
			y->color = RBT_BLACK;
			w->color = RBT_RED;
			w->sub[LR] = y->sub[!LR];
			y->sub[!LR] = w;
			w = path[depth - 1]->sub[!LR] = y;
		}

		/* Case 6 */
		w->color = path[depth - 1]->color;
		path[depth - 1]->color = RBT_BLACK;
		w->sub[!LR]->color = RBT_BLACK;
		path[depth - 1]->sub[!LR] = w->sub[LR];
		w->sub[LR] = path[depth - 1];
		path[depth - 2]->sub[dir[depth - 2]] = w;
		break;
	}
}

static void *HXrbtree_del(struct HXrbtree *btree, const void *key)
{
	struct HXrbtree_node *path[RBT_MAXDEP], *node;
	unsigned char dir[RBT_MAXDEP];
	unsigned int depth = 0;
	void *itemptr;

	if (btree->root == NULL)
		return NULL;

	path[depth]  = reinterpret_cast(struct HXrbtree_node *, &btree->root);
	dir[depth++] = 0;
	node         = btree->root;

	while (node != NULL) {
		int res = btree->super.ops.k_compare(key,
		          node->key, btree->super.key_size);
		if (res == 0)
			break;
		res          = res > 0;
		path[depth]  = node;
		dir[depth++] = res;
		node         = node->sub[res];
	}

	if (node == NULL) {
		errno = ENOENT;
		return NULL;
	}

	/*
	 * Return the data for the node. But it is not going to be useful
	 * if ARBtree was directed to copy it (because it will be released
	 * below.)
	 */
	itemptr = node->data;
	/* Removal of the node from the tree */
	--btree->super.items;
	++btree->tid;

	path[depth] = node;
	if (node->sub[RBT_RIGHT] == NULL)
		/* Simple case: No right subtree, replace by left subtree. */
		path[depth-1]->sub[dir[depth-1]] = node->sub[RBT_LEFT];
	else if (node->sub[RBT_LEFT] == NULL)
		/* Simple case: No left subtree, replace by right subtree. */
		path[depth-1]->sub[dir[depth-1]] = node->sub[RBT_RIGHT];
	else
		/*
		 * Find minimum/maximum element in right/left subtree and
		 * do appropriate deletion while updating @path and @depth.
		 */
		depth = HXrbtree_del_mm(path, dir, depth);

	/*
	 * Deleting a red node does not violate either of the rules, so it is
	 * not necessary to rebalance in such a case.
	 */
	if (node->color == RBT_BLACK)
		HXrbtree_dmov(path, dir, depth);

	if (btree->super.ops.k_free != NULL)
		btree->super.ops.k_free(node->key);
	if (btree->super.ops.d_free != NULL)
		btree->super.ops.d_free(node->data);
	free(node);
	errno = 0;
	/*
	 * In case %HXBT_CDATA was specified, the @itemptr value will be
	 * useless in most cases as it points to freed memory.
	 */
	return itemptr;
}

EXPORT_SYMBOL void *HXmap_del(struct HXmap *xmap, const void *key)
{
	void *vmap = xmap;
	struct HXmap_private *map = vmap;

	switch (map->type) {
	case HXMAPT_HASH:
		return HXhmap_del(vmap, key);
	case HXMAPT_RBTREE:
		return HXrbtree_del(vmap, key);
	default:
		errno = EINVAL;
		return NULL;
	}
}

static void HXhmap_keysvalues(const struct HXhmap *hmap,
    struct HXmap_node *array)
{
	const struct HXhmap_node *node;
	unsigned int i;

	for (i = 0; i < HXhash_primes[hmap->power]; ++i)
		HXlist_for_each_entry(node, &hmap->bk_array[i], anchor) {
			array->key  = node->key;
			array->data = node->data;
			++array;
		}
}

static struct HXmap_node *HXrbtree_keysvalues(const struct HXrbtree_node *node,
    struct HXmap_node *array)
{
	if (node->sub[0] != NULL)
		array = HXrbtree_keysvalues(node->sub[0], array);
	array->key  = node->key;
	array->data = node->data;
	++array;
	if (node->sub[1] != NULL)
		array = HXrbtree_keysvalues(node->sub[1], array);
	return array;
}

EXPORT_SYMBOL struct HXmap_node *HXmap_keysvalues(const struct HXmap *xmap)
{
	const void *vmap = xmap;
	const struct HXmap_private *map = vmap;
	struct HXmap_node *array;

	switch (map->type) {
	case HXMAPT_HASH:
	case HXMAPT_RBTREE:
		break;
	default:
		errno = EINVAL;
		return NULL;
	}

	if ((array = malloc(sizeof(*array) * map->items)) == NULL)
		return NULL;

	switch (map->type) {
	case HXMAPT_HASH:
		HXhmap_keysvalues(vmap, array);
		break;
	case HXMAPT_RBTREE:
		HXrbtree_keysvalues(((struct HXrbtree *)vmap)->root, array);
		break;
	}
	return array;
}

static void *HXhmap_travinit(const struct HXhmap *hmap, unsigned int flags)
{
	struct HXhmap_trav *trav;

	if ((trav = malloc(sizeof(*trav))) == NULL)
		return NULL;
	/* We cannot offer DTRAV. */
	trav->super.flags = flags & ~HXMAP_DTRAV;
	trav->super.type = HXMAPT_HASH;
	trav->hmap = hmap;
	trav->head = NULL;
	trav->bk_current = 0;
	trav->tid = hmap->tid;
	return trav;
}

static void *HXrbtrav_init(const struct HXrbtree *btree, unsigned int flags)
{
	struct HXrbtrav *trav;

	if ((trav = calloc(1, sizeof(*trav))) == NULL)
		return NULL;
	trav->super.flags = flags;
	trav->super.type = HXMAPT_RBTREE;
	trav->tree = btree;
	return trav;
}

EXPORT_SYMBOL struct HXmap_trav *HXmap_travinit(const struct HXmap *xmap,
    unsigned int flags)
{
	const void *vmap = xmap;
	const struct HXmap_private *map = vmap;

	switch (map->type) {
	case HXMAPT_HASH:
		return HXhmap_travinit(vmap, flags);
	case HXMAPT_RBTREE:
		return HXrbtrav_init(vmap, flags);
	default:
		errno = EINVAL;
		return NULL;
	}
}

static const struct HXmap_node *HXhmap_traverse(struct HXhmap_trav *trav)
{
	const struct HXhmap *hmap = trav->hmap;
	const struct HXhmap_node *drop;

	if (trav->head == NULL) {
		trav->head = hmap->bk_array[trav->bk_current].next;
	} else if (trav->tid != hmap->tid) {
		if (trav->bk_current >= HXhash_primes[hmap->power])
			/* bk_array shrunk underneath us, we're done */
			return NULL;
		/*
		 * Reset head so that the while loop will be entered and we
		 * advance to the next bucket.
		 */
		trav->head = &hmap->bk_array[trav->bk_current];
		trav->tid  = hmap->tid;
	} else {
		trav->head = trav->head->next;
	}

	while (trav->head == &hmap->bk_array[trav->bk_current]) {
		if (++trav->bk_current >= HXhash_primes[hmap->power])
			return false;
		trav->head = hmap->bk_array[trav->bk_current].next;
	}

	drop = HXlist_entry(trav->head, struct HXhmap_node, anchor);
	return static_cast(const void *, &drop->key);
}

static void HXrbtrav_checkpoint(struct HXrbtrav *trav,
    const struct HXrbtree_node *node)
{
	const struct HXrbtree *tree = trav->tree;

	if (tree->super.flags & HXMAP_DTRAV) {
		void *old_key = trav->checkpoint;

		trav->checkpoint = tree->super.ops.k_clone(node->key,
		                   tree->super.key_size);
		if (tree->super.ops.k_free != NULL)
			tree->super.ops.k_free(old_key);
	} else {
		trav->checkpoint = node->key;
	}
}

static struct HXrbtree_node *HXrbtrav_next(struct HXrbtrav *trav)
{
	if (trav->current->sub[RBT_RIGHT] != NULL) {
		/* Got a right child */
		struct HXrbtree_node *node;

		trav->dir[trav->depth++] = RBT_RIGHT;
		node = trav->current->sub[RBT_RIGHT];

		/* Which might have left childs (our inorder successors!) */
		while (node != NULL) {
			trav->path[trav->depth] = node;
			node = node->sub[RBT_LEFT];
			trav->dir[trav->depth++] = RBT_LEFT;
		}
		trav->current = trav->path[--trav->depth];
	} else if (trav->depth == 0) {
		/* No right child, no more parents */
		return trav->current = NULL;
	} else if (trav->dir[trav->depth-1] == RBT_LEFT) {
		/* We are the left child of the parent, move on to parent */
		trav->current = trav->path[--trav->depth];
	} else if (trav->dir[trav->depth-1] == RBT_RIGHT) {
		/*
		 * There is no right child, and we are the right child of the
		 * parent, so move on to the next inorder node (a distant
		 * parent). This works by walking up the path until we are the
		 * left child of a parent.
		 */
		while (true) {
			if (trav->depth == 0)
				/* No more parents */
				return trav->current = NULL;
			if (trav->dir[trav->depth-1] != RBT_RIGHT)
				break;
			--trav->depth;
		}
		trav->current = trav->path[--trav->depth];
	}

	HXrbtrav_checkpoint(trav, trav->current);
	return trav->current;
}

static struct HXrbtree_node *HXrbtrav_rewalk(struct HXrbtrav *trav)
{
	/*
	 * When the binary tree has been distorted (or the traverser is
	 * uninitilaized), by either addition or deletion of an object, our
	 * path recorded so far is (probably) invalid too. rewalk() will go and
	 * find the node we were last at.
	 */
	const struct HXrbtree *btree = trav->tree;
	struct HXrbtree_node *node   = btree->root;
	bool go_next = false;

	trav->depth = 0;

	if (trav->current == NULL) {
		/* Walk down the tree to the smallest element */
		while (node != NULL) {
			trav->path[trav->depth] = node;
			node = node->sub[RBT_LEFT];
			trav->dir[trav->depth++] = RBT_LEFT;
		}
	} else {
		/* Search for the specific node to rebegin traversal at. */
		const struct HXrbtree_node *newpath[RBT_MAXDEP];
		unsigned char newdir[RBT_MAXDEP];
		int newdepth = 0, res;
		bool found = false;

		while (node != NULL) {
			newpath[newdepth] = trav->path[trav->depth] = node;
			res = btree->super.ops.k_compare(trav->checkpoint,
			      node->key, btree->super.key_size);
			if (res == 0) {
				++trav->depth;
				found = true;
				break;
			}
			res = res > 0;
			trav->dir[trav->depth++] = res;

			/*
			 * This (working) code gets 1st place in being totally
			 * cryptic without comments, so here goes:
			 *
			 * Right turns do not need to be saved, because we do
			 * not need to stop at that particular node again but
			 * can go directly to the next in-order successor,
			 * which must be a parent somewhere upwards where we
			 * did a left turn. If we only ever did right turns,
			 * we would be at the last node already.
			 *
			 * Imagine a 32-element perfect binary tree numbered
			 * from 1..32, and walk to 21 (directions: RLRL).
			 * The nodes stored are 24 and 22. btrav_next will
			 * go to 22, do 23, then jump _directly_ back to 24,
			 * omitting the redundant check at 20.
			 */
			if (res == RBT_LEFT)
				newdir[newdepth++] = RBT_LEFT;

			node = node->sub[res];
		}

		if (found) {
			/*
			 * We found the node, but which HXbtraverse() has
			 * already returned. Advance to the next inorder node.
			 * (Code needs to come after @current assignment.)
			 */
			go_next = true;
		} else {
			/*
			 * If the node travp->current is actually deleted (@res
			 * will never be 0 above), traversal re-begins at the
			 * next inorder node, which happens to be the last node
			 * we turned left at.
			 */
			memcpy(trav->path, newpath, sizeof(trav->path));
			memcpy(trav->dir, newdir, sizeof(trav->dir));
			trav->depth = newdepth;
		}
	}

	if (trav->depth == 0) {
		/* no more elements */
		trav->current = NULL;
	} else {
		trav->current = trav->path[--trav->depth];
		if (trav->current == NULL)
			fprintf(stderr, "btrav_rewalk: problem: current==NULL\n");
		HXrbtrav_checkpoint(trav, trav->current);
	}

	trav->tid = btree->tid;
	if (go_next)
		return HXrbtrav_next(trav);
	else
		return trav->current;
}

static const struct HXmap_node *HXrbtree_traverse(struct HXrbtrav *trav)
{
	const struct HXrbtree_node *node;

	if (trav->tid != trav->tree->tid || trav->current == NULL)
		/*
		 * Every HXrbtree operation that significantly changes the
		 * B-tree, increments @tid so we can decide here to rewalk.
		 */
		node = HXrbtrav_rewalk(trav);
	else
		node = HXrbtrav_next(trav);

	return (node != NULL) ? static_cast(const void *, &node->key) : NULL;
}

EXPORT_SYMBOL const struct HXmap_node *HXmap_traverse(struct HXmap_trav *trav)
{
	void *xtrav = trav;

	if (xtrav == NULL)
		return NULL;

	switch (trav->type) {
	case HXMAPT_HASH:
		return HXhmap_traverse(xtrav);
	case HXMAPT_RBTREE:
		return HXrbtree_traverse(xtrav);
	default:
		errno = EINVAL;
		return NULL;
	}
}

static void HXrbtrav_free(struct HXrbtrav *trav)
{
	const struct HXmap_private *super = &trav->tree->super;

	if ((super->flags & HXMAP_DTRAV) && super->ops.k_free != NULL)
		super->ops.k_free(trav->checkpoint);
	free(trav);
}

EXPORT_SYMBOL void HXmap_travfree(struct HXmap_trav *trav)
{
	void *xtrav = trav;

	if (xtrav == NULL)
		return;
	switch (trav->type) {
	case HXMAPT_RBTREE:
		HXrbtrav_free(xtrav);
		break;
	default:
		free(xtrav);
		break;
	}
}

static void HXhmap_qfe(const struct HXhmap *hmap, qfe_fn_t fn, void *arg)
{
	const struct HXhmap_node *hnode;
	unsigned int i;

	for (i = 0; i < HXhash_primes[hmap->power]; ++i)
		HXlist_for_each_entry(hnode, &hmap->bk_array[i], anchor)
			if (!(*fn)(static_cast(const void *, &hnode->key), arg))
				return;
}

static void HXrbtree_qfe(const struct HXrbtree_node *node,
    qfe_fn_t fn, void *arg)
{
	if (node->sub[RBT_LEFT] != NULL)
		HXrbtree_qfe(node->sub[RBT_LEFT], fn, arg);
	if (!(*fn)(static_cast(const void *, &node->key), arg))
		return;
	if (node->sub[RBT_RIGHT] != NULL)
		HXrbtree_qfe(node->sub[RBT_RIGHT], fn, arg);
}

EXPORT_SYMBOL void HXmap_qfe(const struct HXmap *xmap, qfe_fn_t fn, void *arg)
{
	const void *vmap = xmap;
	const struct HXmap_private *map = vmap;

	switch (map->type) {
	case HXMAPT_HASH:
		HXhmap_qfe(vmap, fn, arg);
		errno = 0;
		break;
	case HXMAPT_RBTREE: {
		const struct HXrbtree *tree = vmap;
		if (tree->root != NULL)
			HXrbtree_qfe(tree->root, fn, arg);
		errno = 0;
		break;
	}
	default:
		errno = EINVAL;
	}
}
