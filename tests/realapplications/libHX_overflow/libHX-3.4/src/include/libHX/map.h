#ifndef _LIBHX_MAP_H
#define _LIBHX_MAP_H 1

#include <sys/types.h>
#ifndef __cplusplus
#	include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Abstract:
 * %HXMAPT_DEFAULT:	whatever is fast and maps k-v, without preference
 * %HXMAPT_ORDERED:	anything ordered, no further preference
 *
 * Specific:
 * %HXMAPT_HASH:	map based on hash
 * %HXMAPT_RBTREE:	map based on red-black binary tree
 */
enum HXmap_type {
	HXMAPT_HASH = 1,
	HXMAPT_RBTREE,

	/* aliases - assignments may change */
	HXMAPT_DEFAULT = HXMAPT_HASH,
	HXMAPT_ORDERED = HXMAPT_RBTREE,
};

/**
 * Flags changable at runtime:
 * %HXMAP_NOREPLACE:	Calling HXmap_add() for an already existing key will
 * 			throw an error (no-overwrite semantics)
 *
 * Initialization-time flags only:
 * %HXMAP_SINGULAR:	Instead of an associative map, provide a set
 * %HXMAP_SKEY:		Key will be a C-style string (sets ops->k_*)
 * %HXMAP_CKEY:		Make a copy of the key on HXmap_add
 * %HXMAP_SDATA:	Data will be a C-style string (presets ops->d_*)
 * %HXMAP_CDATA:	Make a copy of the data on HXmap_add
 */
enum {
	HXMAP_NOREPLACE = 1 << 0,
	HXMAP_SINGULAR  = 1 << 27,
	HXMAP_SKEY      = 1 << 28,
	HXMAP_CKEY      = 1 << 29,
	HXMAP_SDATA     = 1 << 30,
	HXMAP_CDATA     = 1 << 31,

	HXMAP_SCKEY     = HXMAP_SKEY | HXMAP_CKEY,
	HXMAP_SCDATA    = HXMAP_SDATA | HXMAP_CDATA,
};

/**
 * Flags for the traverser
 * %HXMAP_DTRAV:	Support deletion of elements while traversing
 */
enum {
	HXMAP_DTRAV     = 1 << 0,
};

struct HXmap_trav;

/**
 * @items:	number of items in the map
 * @flags:	flags for this map
 */
struct HXmap {
	unsigned int items, flags;
};

struct HXmap_ops {
	/* k_compare: the size argument is needed for memcmp. */
	int (*k_compare)(const void *, const void *, size_t);
	void *(*k_clone)(const void *, size_t);
	void (*k_free)(void *);
	void *(*d_clone)(const void *, size_t);
	void (*d_free)(void *);
	unsigned long (*k_hash)(const void *, size_t);
};

struct HXmap_node {
	union {
		void *key;
		const char *const skey;
	};
	union {
		void *data;
		char *sdata;
	};
};

extern struct HXmap *HXmap_init(enum HXmap_type, unsigned int);
extern struct HXmap *HXmap_init5(enum HXmap_type, unsigned int,
	const struct HXmap_ops *, size_t, size_t);

extern int HXmap_add(struct HXmap *, const void *, const void *);
extern const struct HXmap_node *HXmap_find(const struct HXmap *, const void *);
extern void *HXmap_get(const struct HXmap *, const void *);
extern void *HXmap_del(struct HXmap *, const void *);
extern struct HXmap_node *HXmap_keysvalues(const struct HXmap *);
extern struct HXmap_trav *HXmap_travinit(const struct HXmap *, unsigned int);
extern const struct HXmap_node *HXmap_traverse(struct HXmap_trav *);
extern void HXmap_travfree(struct HXmap_trav *);
extern void HXmap_qfe(const struct HXmap *,
	bool (*)(const struct HXmap_node *, void *), void *);
extern void HXmap_free(struct HXmap *);

extern unsigned long HXhash_jlookup3(const void *, size_t);
extern unsigned long HXhash_jlookup3s(const void *, size_t);
extern unsigned long HXhash_djb2(const void *, size_t);

#ifdef __cplusplus
} /* extern "C" */

extern "C++" {

template<typename type> static inline type
HXmap_get(const struct HXmap *map, const void *key)
{
	return reinterpret_cast<type>(HXmap_get(map, key));
}

template<typename type> static inline type
HXmap_del(struct HXmap *map, const void *key)
{
	return reinterpret_cast<type>(HXmap_del(map, key));
}

}
#endif

#endif /* _LIBHX_MAP_H */
