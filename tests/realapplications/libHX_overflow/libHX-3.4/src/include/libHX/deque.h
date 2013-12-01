#ifndef _LIBHX_DEQUE_H
#define _LIBHX_DEQUE_H 1

#ifdef __cplusplus
#	include <cstdlib>
#else
#	include <stdlib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct HXdeque_node {
	struct HXdeque_node *next;
	union {
		void *ptr;
		char *sptr;
	};
	struct HXdeque *parent;
	struct HXdeque_node *prev;
};

struct HXdeque {
	struct HXdeque_node *first;
	void *ptr;
	struct HXdeque_node *last;
	unsigned int items;
};

extern struct HXdeque *HXdeque_init(void);
extern struct HXdeque_node *HXdeque_push(struct HXdeque *, const void *);
extern struct HXdeque_node *HXdeque_unshift(struct HXdeque *, const void *);
extern void *HXdeque_pop(struct HXdeque *);
extern void *HXdeque_shift(struct HXdeque *);
extern void HXdeque_move(struct HXdeque_node *, struct HXdeque_node *);
extern struct HXdeque_node *HXdeque_find(struct HXdeque *, const void *);
extern void *HXdeque_get(struct HXdeque *, const void *);
extern void *HXdeque_del(struct HXdeque_node *);
extern void HXdeque_free(struct HXdeque *);
extern void HXdeque_genocide2(struct HXdeque *, void (*)(void *));
extern void **HXdeque_to_vec(const struct HXdeque *, unsigned int *);

static inline void HXdeque_genocide(struct HXdeque *dq)
{
	HXdeque_genocide2(dq, free);
}

#ifdef __cplusplus
} /* extern "C" */

extern "C++" {

template<typename type> static inline type HXdeque_pop(struct HXdeque *dq)
{
	return reinterpret_cast<type>(HXdeque_pop(dq));
}

template<typename type> static inline type HXdeque_shift(struct HXdeque *dq)
{
	return reinterpret_cast<type>(HXdeque_shift(dq));
}

template<typename type> static inline type
HXdeque_get(struct HXdeque *dq, const void *ptr)
{
	return reinterpret_cast<type>(HXdeque_get(dq, ptr));
}

template<typename type> static inline type
HXdeque_del(struct HXdeque_node *nd)
{
	return reinterpret_cast<type>(HXdeque_del(nd));
}

template<typename type> static inline type *
HXdeque_to_vec(struct HXdeque *dq, unsigned int *n)
{
	return reinterpret_cast<type *>(HXdeque_to_vec(dq, n));
}

} /* extern "C++" */
#endif

#endif /* _LIBHX_DEQUE_H */
