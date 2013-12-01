/*
 *	This program is in the Public Domain
 */
#include <sys/resource.h>
#include <sys/time.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <libHX/init.h>
#include <libHX/map.h>
#include <libHX/misc.h>
#include <libHX/string.h>
#include "internal.h"
#include "map_int.h"

union HXpoly {
	struct HXmap *map;
	struct HXhmap *hmap;
	struct HXrbtree *rbt;
};

typedef struct HXmap *(*map_create4_fn_t)(unsigned int,
	const struct HXmap_ops *, size_t, size_t);

static unsigned int tmap_indent = 0;

static inline void tmap_ipush(void)
{
	++tmap_indent;
}

static inline void tmap_ipop(void)
{
	if (tmap_indent > 0)
		--tmap_indent;
}

static void tmap_printf(const char *fmt, ...)
{
	unsigned int i;
	va_list args;

	for (i = 0; i < tmap_indent; ++i)
		printf("\t");
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

static void tmap_time(struct timeval *tv)
{
	struct rusage r;
	if (getrusage(RUSAGE_SELF, &r) == 0)
		*tv = r.ru_utime;
}

static unsigned int tmap_smart_rand(unsigned int *left, unsigned int *right)
{
	unsigned int z = HX_irand(*left, *right);

	if (z == *left)
		++*left;
	else if (z == *right - 1)
		--*right;
	return z;
}

/**
 * tmap_rword - create random word
 * @dest:	char buffer
 * @length:	size of buffer
 */
static inline void tmap_rword(char *dest, unsigned int length)
{
	while (--length > 0)
		*dest++ = HX_irand('a', 'z' + 1);
	*dest = '\0';
}

static void tmap_add_rand(struct HXmap *map, unsigned int num)
{
	char key[8], value[HXSIZEOF_Z32];

	while (num-- > 0) {
		tmap_rword(key, sizeof(key));
		snprintf(value, sizeof(value), "%u", num);
		if (HXmap_add(map, key, value) == -EEXIST)
			++num;
	}
}

static void tmap_flush(struct HXmap *map, bool verbose)
{
	const struct HXmap_node *node;
	struct HXmap_trav *iter;

	tmap_printf("Flushing %u elements (with traversal)\n", map->items);
	tmap_ipush();
	while (map->items != 0) {
		/* May need to reload traverser due to deletion */
		if (verbose)
			tmap_printf("Restarting traverser\n");
		if ((iter = HXmap_travinit(map, HXMAP_DTRAV)) == NULL)
			break;
		tmap_ipush();
		while ((node = HXmap_traverse(iter)) != NULL) {
			if (verbose)
				tmap_printf("Destroying {%s, %s}\n",
					node->skey, node->sdata);
			HXmap_del(map, node->key);
		}
		tmap_ipop();
		HXmap_travfree(iter);
	}
	tmap_ipop();
}

static void tmap_add_speed(struct HXmap *map)
{
	struct timeval start, stop, delta;
	unsigned int threshold;

	tmap_printf("MAP test 1: Timing add operation\n");
	tmap_ipush();
	tmap_time(&start);
	do {
		tmap_add_rand(map, 1);
		tmap_time(&stop);
		HX_diff_timeval(&delta, &stop, &start);
	} while (!(delta.tv_sec >= 1 || map->items >= 1000000));
	tmap_printf("%u elements in %ld.%06ld "
		"(plus time measurement overhead)\n",
		map->items, static_cast(long, delta.tv_sec),
		static_cast(long, delta.tv_usec));
	threshold = map->items;
	tmap_flush(map, false);

	tmap_time(&start);
	tmap_add_rand(map, threshold);
	tmap_time(&stop);
	HX_diff_timeval(&delta, &stop, &start);
	tmap_printf("%u elements in %ld.%06ld "
		"(w/o overhead)\n",
		map->items, static_cast(long, delta.tv_sec),
		static_cast(long, delta.tv_usec));
	tmap_ipop();
}

static bool tmap_each_fn(const struct HXmap_node *node, void *arg)
{
	return true;
}

static void tmap_trav_speed(struct HXmap *map)
{
	struct timeval start, stop, delta, delta2;
	const struct HXmap_node *node;
	struct HXmap_trav *iter;

	tmap_printf("MAP test 2: Timing traversal\n");
	tmap_ipush();
	iter = HXmap_travinit(map, 0);
	tmap_time(&start);
	while ((node = HXmap_traverse(iter)) != NULL)
		;
	tmap_time(&stop);
	HX_diff_timeval(&delta, &stop, &start);
	HXmap_travfree(iter);
	tmap_printf("Open traversal of %u nodes: %ld.%06lds\n",
		map->items, static_cast(long, delta.tv_sec),
		static_cast(long, delta.tv_usec));

	tmap_time(&start);
	HXmap_qfe(map, tmap_each_fn, NULL);
	tmap_time(&stop);
	HX_diff_timeval(&delta, &stop, &start);
	tmap_printf("QFE traversal of %u nodes: %ld.%06lds\n",
		map->items, static_cast(long, delta.tv_sec),
		static_cast(long, delta.tv_usec));
	tmap_ipop();

	tmap_printf("MAP test 2a: Timing lookup\n");
	tmap_ipush();
	iter = HXmap_travinit(map, 0);
	tmap_time(&start);
	while ((node = HXmap_traverse(iter)) != NULL)
		HXmap_find(map, node->key);
	tmap_time(&stop);
	HX_diff_timeval(&delta2, &stop, &start);
	HXmap_travfree(iter);
	/* delta2 includes traversal time */
	start = delta;
	stop  = delta2;
	HX_diff_timeval(&delta, &stop, &start);
	tmap_printf("Lookup of %u nodes: %ld.%06lds\n",
		map->items, static_cast(long, delta.tv_sec),
		static_cast(long, delta.tv_usec));
	tmap_ipop();
}

static void tmap_flat(const struct HXmap *map)
{
	struct HXmap_node *nodes;
	unsigned int i;

	tmap_printf("Retrieving flattened list of %u elements:\n", map->items);
	tmap_ipush();
	nodes = HXmap_keysvalues(map);
	if (nodes == NULL) {
		perror("HXmap_keysvalues");
		abort();
	}
	for (i = 0; i < map->items; ++i)
		tmap_printf("%u. %s -> %s\n", i, nodes[i].key, nodes[i].data);
	tmap_ipop();
	free(nodes);
}

static void tmap_trav(struct HXmap *map)
{
	const struct HXmap_node *node;
	unsigned int i = ~(~0U >> 1);
	char key[8], value[HXSIZEOF_Z32];
	struct HXmap_trav *iter;

	tmap_printf("Simple traversal:\n");
	tmap_ipush();
	iter = HXmap_travinit(map, 0);
	while ((node = HXmap_traverse(iter)) != NULL)
		tmap_printf("%s -> %s\n", node->skey, node->sdata);
	tmap_ipop();
	HXmap_travfree(iter);

	tmap_printf("Add modification during traversal:\n");
	tmap_ipush();
	iter = HXmap_travinit(map, 0);
	while ((node = HXmap_traverse(iter)) != NULL) {
		tmap_printf("%s -> %s\n", node->skey, node->sdata);
		tmap_rword(key, sizeof(key));
		snprintf(value, sizeof(value), "%u", i++);
		HXmap_add(map, key, value);
	}
	tmap_ipop();
	HXmap_travfree(iter);
}

static void tmap_generic_tests(enum HXmap_type type,
    unsigned long (*hash_fn)(const void *, size_t), const char *hash_name)
{
	struct HXmap_ops ops = {.k_hash = hash_fn};
	struct HXmap *map;

	tmap_printf("Using hash %s\n", hash_name);
	map = HXmap_init5(type, HXMAP_SCKEY | HXMAP_SCDATA | HXMAP_NOREPLACE,
	      &ops, 0, 0);
	tmap_add_speed(map);
	tmap_trav_speed(map);
	tmap_flush(map, false);

	tmap_add_rand(map, 2);
	tmap_flat(map);
	tmap_trav(map);
	tmap_flush(map, true);
	HXmap_free(map);
}

static int tmap_strtolcmp(const void *a, const void *b, size_t z)
{
	return strtol(static_cast(const char *, a), NULL, 0) -
	       strtol(static_cast(const char *, b), NULL, 0);
}

static const struct HXmap_ops tmap_nstr_ops = {
	.k_compare = tmap_strtolcmp,
	.k_hash    = HXhash_djb2,
};

static const struct HXmap_ops tmap_nstr_l3_ops = {
	.k_compare = tmap_strtolcmp,
	.k_hash    = HXhash_jlookup3s,
};

static const struct HXmap_ops tmap_words_ops = {
	.k_hash = HXhash_djb2,
};

static const struct HXmap_ops tmap_words_l3_ops = {
	.k_hash = HXhash_jlookup3s,
};

/**
 * tmap_expect - compare two strings or warn
 * @result:	result from previous operations
 * @expected:	what we think should have happened
 */
static int tmap_expect(const char *result, const char *expected)
{
	int ret = strcmp(result, expected);
	tmap_ipush();
	tmap_printf("Expected: %s\n", expected);
	tmap_printf("  Result: %s\n", result);
	if (ret != 0) {
		tmap_ipush();
		tmap_printf("...failed\n");
		tmap_ipop();
	}
	tmap_ipop();
	return ret;
}

/**
 * tmap_new_perfect_tree -
 * Add elements in such a way that it does not cause an rbtree to rebalance and
 * thus deterministically attain a perfect binary tree. For hash maps, it only
 * serves to add some elements.
 */
static void tmap_new_perfect_tree(struct HXmap *map,
    unsigned int height, unsigned int mult)
{
	unsigned int right = 1 << height;
	unsigned int incr  = right;
	unsigned int left  = incr / 2;
	unsigned int y, x;
	char buf[HXSIZEOF_Z32];

	for (y = 0; y < height; ++y) {
		for (x = left; x < right; x += incr) {
			snprintf(buf, sizeof(buf), "%u", x * mult);
			HXmap_add(map, buf, NULL);
		}
		incr /= 2;
		left /= 2;
	}
}

/**
 * Compute an "agglomeration" index that models the lack of distributedness
 * in hash maps. Range is 0-100%.
 */
static double hmap_agg_index(const struct HXhmap *hmap, bool verbose)
{
	const struct HXhmap_node *hnode;
	unsigned int i;
	int f = 0, j;

	if (hmap->super.items == 1)
		return 0;
	if (verbose)
		printf("{");
	for (i = 0; i < HXhash_primes[hmap->power]; ++i) {
		j = 0;
		HXlist_for_each_entry(hnode, &hmap->bk_array[i], anchor)
			++j;
		if (verbose)
			printf("%u,", j);
		/* Difference to ideal is abs(1 - j): */
		--j;
		if (j < 0)
			j = -j;
		f += j;
	}
	if (verbose)
		printf("}\n");
	/* Ignore buckets that must logically be empty (pigeonhole principle) */
	f -= HXhash_primes[hmap->power] - hmap->super.items;
	/* It's f/(2(e-1)) */
	/* Now return % */
	return static_cast(double, 50 * f) / (hmap->super.items - 1);
}

/**
 * Test one hash function with different keys and check agglomeration index.
 */
static void tmap_hmap_test_1a(const char *map_type,
    unsigned long (*hash_fn)(const void *, size_t), unsigned int max_power)
{
	struct HXmap_ops intstr_ops = {
		.k_compare = tmap_strtolcmp,
		.k_hash    = hash_fn,
	};
	struct HXmap_ops words_ops = {
		.k_hash    = hash_fn,
	};
	unsigned int power;
	union HXpoly u;

	for (power = 1; power <= max_power; ++power) {
		u.map = HXmap_init5(HXMAPT_HASH, HXMAP_SCKEY,
		        &intstr_ops, 0, 0);
		tmap_new_perfect_tree(u.map, power, 2);
		tmap_printf("%s, intstr, %u items/%u buckets, "
			"agglomeration: %.2f%%\n", map_type,
			u.map->items, HXhash_primes[u.hmap->power],
			hmap_agg_index(u.hmap, false));
		HXmap_free(u.map);
	}

	u.map = HXmap_init5(HXMAPT_HASH, HXMAP_SCKEY, &words_ops, 0, 0);
	while (u.map->items < 1 << max_power) {
		/* Fill up just right up to the maximum load */
		tmap_add_rand(u.map, u.hmap->max_load - u.map->items);
		tmap_printf("%s, words, %u items/%u buckets, "
			"agglomeration: %.2f%%\n", map_type,
			u.map->items, HXhash_primes[u.hmap->power],
			hmap_agg_index(u.hmap, false));
		/* trigger resize */
		tmap_add_rand(u.map, 1);
		tmap_printf("%s, words, %u items/%u buckets, "
			"agglomeration: %.2f%%\n", map_type,
			u.map->items, HXhash_primes[u.hmap->power],
			hmap_agg_index(u.hmap, false));
	}
	HXmap_free(u.map);
}

/**
 * tmap_hmap_test_1 - test distributedness of elements
 */
static void tmap_hmap_test_1(void)
{
	static const unsigned int max_power = 15;

	tmap_printf("HMAP test 1A: Hashmap distribution\n");
	tmap_ipush();
	tmap_hmap_test_1a("DJB2", HXhash_djb2, max_power);
	tmap_hmap_test_1a("JL3", HXhash_jlookup3s, max_power);
	tmap_ipop();
}

static void __rbt_walk_tree(const struct HXrbtree_node *node,
    char *buf, size_t s)
{
	bool has_children = node->sub[0] != NULL || node->sub[1] != NULL;
	HX_strlcat(buf, node->skey, s);

	if (node->color == RBT_BLACK)
		HX_strlcat(buf, "%b", s);
	if (has_children)
		HX_strlcat(buf, "(" /* ) */, s);
	if (node->sub[0] != NULL)
		__rbt_walk_tree(node->sub[0], buf, s);
	if (node->sub[1] != NULL) {
		HX_strlcat(buf, ",", s);
		__rbt_walk_tree(node->sub[1], buf, s);
	}
	if (has_children)
		HX_strlcat(buf, /* ( */ ")", s);
}

/**
 * rbt_walk_tree - walk the tree and provide a string suitable for texitree
 * @node:	node of an rbtree to start diving at
 * @buf:	buffer for texitree representation
 * @size:	size for @buf
 */
static void rbt_walk_tree(const struct HXrbtree_node *node,
    char *buf, size_t size)
{
	*buf = '\0';
	__rbt_walk_tree(node, buf, size);
}

/**
 * rbt_new_perfect_tree - generate a perfect binary tree
 * @height:	height of the desired tree
 * @mult:	multiplicator for node numbers
 *
 * Produces a tree of desired height with exactly 2^height-1 nodes.
 */
static struct HXmap *rbt_new_perfect_tree(unsigned int height,
    unsigned int mult)
{
	struct HXmap *tree =
		HXmap_init5(HXMAPT_RBTREE, HXMAP_SCKEY, &tmap_nstr_ops, 0, 0);
	tmap_new_perfect_tree(tree, height, mult);
	return tree;
}

static unsigned int rbt_tree_height(const struct HXrbtree_node *node)
{
	unsigned int a = 1, b = 1;
	if (node->sub[0] != NULL)
		a += rbt_tree_height(node->sub[0]);
	if (node->sub[1] != NULL)
		b += rbt_tree_height(node->sub[1]);
	return (a > b) ? a : b;
}

static void rbt_height_check(const struct HXrbtree *tree)
{
	double min, max, avg;
	min = log(tree->super.items + 1) / log(2);
	max = 2 * log(tree->super.items + 1) / log(2);
	avg = log((pow(2, min) + pow(2, max)) / 2) / log(2);
	tmap_ipush();
	tmap_printf("%u items; height %u; min/avg/max %.2f/%.2f/%.2f\n",
		tree->super.items, rbt_tree_height(tree->root),
		min, avg, max);
	tmap_ipop();
}

/**
 * tmap_rbt_test_1 - basic rbt node layout tests
 */
static void tmap_rbt_test_1(void)
{
	union HXpoly u;
	char buf[80];

	tmap_printf("RBT test 1A: Creating tree with 7 nodes (height 3)\n");
	u.map = rbt_new_perfect_tree(3, 2);

	tmap_printf("RBT test 1B: Manual traverse\n");
	rbt_walk_tree(u.rbt->root, buf, sizeof(buf));

	tmap_printf("RBT test 1C: Check for correct positions and colors\n");
	tmap_expect(buf, "8%b(4%b(2,6),12%b(10,14))");

	/*        8
	 *      /     \
	 *    4         12
	 *   / \       /  \
	 *  2   6    10    14
	 *          /
	 *         9
	 */
	tmap_printf("RBT test 1D: Node insertion and test for positions/colors\n");
	HXmap_add(u.map, "9", NULL);
	rbt_walk_tree(u.rbt->root, buf, sizeof(buf));
	tmap_expect(buf, "8%b(4%b(2,6),12(10%b(9),14%b))");

	tmap_printf("RBT test 1E: Height check\n");
	rbt_height_check(u.rbt);

	tmap_printf("RBT test 1G: Node deletion\n");
	HXmap_del(u.map, "8");
	rbt_walk_tree(u.rbt->root, buf, sizeof(buf));
	tmap_expect(buf, "9%b(4%b(2,6),12(10%b,14%b))");

	/*        9       (8 replaced by its in-order successor 9)
	 *      /    \
	 *    4        12
	 *   / \      /  \
	 *  2   6   10    14
	 */
	HXmap_free(u.map);
}

/**
 * rbt_no_2red_children - verify rbtree rule
 * @node:	subtree to verify
 *
 * Verify that there are no red nodes with red children.
 */
static bool rbt_no_2red_children(const struct HXrbtree_node *node)
{
	if (node->sub[RBT_LEFT] != NULL) {
		if (node->color == RBT_RED &&
		    node->sub[RBT_LEFT]->color == RBT_RED)
			return false;
		if (!rbt_no_2red_children(node->sub[RBT_LEFT]))
			return false;
	}
	if (node->sub[RBT_RIGHT] != NULL) {
		if (node->color == RBT_RED &&
		    node->sub[RBT_RIGHT]->color == RBT_RED)
			return false;
		if (!rbt_no_2red_children(node->sub[RBT_RIGHT]))
			return false;
	}
	return true;
}

/**
 * rbt_black_height - calculate the black height of a tree
 * @node:	subtree to find the black height for
 *
 * Returns the black height, or -1 if the black height is not consistent.
 */
static int rbt_black_height(const struct HXrbtree_node *node)
{
	int lh = 0, rh = 0;

	if (node->sub[RBT_LEFT] != NULL)
		if ((lh = rbt_black_height(node->sub[RBT_LEFT])) == -1)
			return -1;
	if (node->sub[RBT_RIGHT] != NULL)
		if ((rh = rbt_black_height(node->sub[RBT_RIGHT])) == -1)
			return -1;
	if (node->sub[RBT_LEFT] != NULL && node->sub[RBT_RIGHT] != NULL)
		if (lh != rh)
			return -1;
	if (node->sub[RBT_LEFT] != NULL)
		return lh + (node->color == RBT_BLACK);
	else
		return rh + (node->color == RBT_BLACK);
}

static bool rbt_verify_tree(const struct HXrbtree_node *root)
{
	/* Root is black */
	if (root->color != RBT_BLACK) {
		tmap_printf("Root is not black\n");
		return false;
	}
	/* A red node may not have any red children */
	if (!rbt_no_2red_children(root)) {
		tmap_printf("Red node may not have red children violated\n");
		return false;
	}
	/* Black height must be consistent */
	if (rbt_black_height(root) < 0) {
		tmap_printf("Black height violated\n");
		return false;
	}
	return true;
}

/**
 * rbt_peel_tree - slowly destroy tree and check characteristics
 * @tree:	the object to disseminate
 * @range:	original number of elements in the tree
 */
static void rbt_peel_tree(union HXpoly u, unsigned int range)
{
	unsigned int left = 1;

	while (u.map->items != 0) {
		unsigned long number = tmap_smart_rand(&left, &range);

		HXmap_del(u.map, reinterpret_cast(const void *, number));
		if (errno == -ENOENT)
			continue;
		if (u.map->items == 0)
			break;
		if (!rbt_verify_tree(u.rbt->root))
			return;
	}
}

static void tmap_rbt_test_7(void)
{
	unsigned int i, elems, order, left, right;
	union HXpoly u;
	int ret;

	tmap_printf("RBT test 7: AMOV/DMOV\n");
	tmap_ipush();
	u.map = HXmap_init(HXMAPT_RBTREE, 0);
	for (order = 2; order <= 10; ++order) {
		elems = (1 << order) - 1;
		tmap_printf("Tree of order %u [e=%u]\n", order, elems);

		/* Build a random tree */
		left  = 1;
		right = elems + 1;
		for (i = 0; i < elems; ++i) {
			unsigned long z = tmap_smart_rand(&left, &right);

			ret = HXmap_add(u.map,
			      reinterpret_cast(const void *, z), NULL);
			if (ret == -EEXIST)
				--i;
			if (!rbt_verify_tree(u.rbt->root))
				tmap_printf("Verification failed\n");
		}
		/* Dismantle. */
		rbt_height_check(u.rbt);
		rbt_peel_tree(u, elems + 1);
	}
	tmap_ipop();
	HXmap_free(u.map);
}

int main(void)
{
	if (HX_init() <= 0)
		abort();
	tmap_printf("* HXhashmap\n");
	tmap_generic_tests(HXMAPT_HASH, HXhash_djb2, "DJB2");
	tmap_generic_tests(HXMAPT_HASH, HXhash_jlookup3s, "JL3");
	tmap_hmap_test_1();

	tmap_printf("\n* RBtree\n");
	tmap_generic_tests(HXMAPT_RBTREE, NULL, "<NONE>");
	tmap_rbt_test_1();
	tmap_rbt_test_7();

	HX_exit();
	return EXIT_SUCCESS;
}
