/*
 *	Auto-sizing memory containers
 *	Copyright © Jan Engelhardt <jengelh [at] medozas de>, 2002 - 2008
 *
 *	This file is part of libHX. libHX is free software; you can
 *	redistribute it and/or modify it under the terms of the GNU
 *	Lesser General Public License as published by the Free Software
 *	Foundation; either version 2.1 or 3 of the License.
 */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libHX/string.h>
#include "internal.h"

#define HXMC_IDENT 0x200571AF

struct memcont {
	size_t alloc, length;
	unsigned int id;
	char data[0];
};

static inline size_t __HXmc_request(size_t len)
{
	/* The container, data portion, and a trailing \0 */
	return sizeof(struct memcont) + len + 1;
}

static inline void HXmc_check(const struct memcont *c)
{
	if (c->id != HXMC_IDENT)
		fprintf(stderr, "libHX-mc error: not a hxmc object!\n");
}

static inline struct memcont *HXmc_base(const hxmc_t *p)
{
	return containerof(p, struct memcont, data);
}

EXPORT_SYMBOL hxmc_t *HXmc_strinit(const char *s)
{
	hxmc_t *t = NULL;
	return HXmc_memcpy(&t, s, strlen(s));
}

EXPORT_SYMBOL hxmc_t *HXmc_meminit(const void *ptr, size_t len)
{
	hxmc_t *t = NULL;
	return HXmc_memcpy(&t, ptr, len);
}

EXPORT_SYMBOL hxmc_t *HXmc_strcpy(hxmc_t **vp, const char *s)
{
	if (s == NULL) {
		HXmc_free(*vp);
		*vp = NULL;
		return NULL;
	}
	return HXmc_memcpy(vp, s, strlen(s));
}

EXPORT_SYMBOL hxmc_t *HXmc_memcpy(hxmc_t **vp, const void *ptr, size_t len)
{
	struct memcont *ctx;
	if (*vp != NULL) {
		ctx = HXmc_base(*vp);
		HXmc_check(ctx);
		if (ctx->alloc < len) {
			ctx = realloc(ctx, __HXmc_request(len));
			if (ctx == NULL)
				return NULL;
			ctx->alloc = len;
		}
	} else {
		ctx = malloc(__HXmc_request(len));
		if (ctx == NULL)
			return NULL;
		ctx->id    = HXMC_IDENT;
		ctx->alloc = len;
	}

	if (ptr == NULL) {
		ctx->length  = 0;
		ctx->data[0] = '\0';
		return *vp = ctx->data;
	}

	memcpy(ctx->data, ptr, ctx->length = len);
	ctx->data[len] = '\0';
	return *vp = ctx->data;
}

EXPORT_SYMBOL size_t HXmc_length(const hxmc_t *vp)
{
	const struct memcont *ctx;

	if (vp == NULL)
		return 0;
	ctx = HXmc_base(vp);
	HXmc_check(ctx);
	return ctx->length;
}

EXPORT_SYMBOL hxmc_t *HXmc_setlen(hxmc_t **vp, size_t len)
{
	struct memcont *ctx;
	if (HXmc_trunc(vp, len) == NULL)
		return NULL;

	ctx = HXmc_base(*vp);
	ctx->length = len;
	return *vp;
}

EXPORT_SYMBOL hxmc_t *HXmc_trunc(hxmc_t **vp, size_t len)
{
	struct memcont *ctx = HXmc_base(*vp);

	HXmc_check(ctx);
	if (len > ctx->alloc) {
		ctx = realloc(ctx, __HXmc_request(len));
		if (ctx == NULL)
			return NULL;
		ctx->alloc = len;
	} else {
		ctx->data[len] = '\0';
		ctx->length = len;
	}
	return *vp = ctx->data;
}

EXPORT_SYMBOL hxmc_t *HXmc_strcat(hxmc_t **vp, const char *s)
{
	if (s == NULL)
		return *vp;
	return HXmc_memcat(vp, s, strlen(s));
}

EXPORT_SYMBOL hxmc_t *HXmc_memcat(hxmc_t **vp, const void *ptr, size_t len)
{
	struct memcont *ctx = HXmc_base(*vp);
	size_t nl = ctx->length + len;

	HXmc_check(ctx);
	if (nl > ctx->alloc) {
		ctx = realloc(ctx, __HXmc_request(nl));
		if (ctx == NULL)
			return NULL;
		ctx->alloc = nl;
	}
	if (ptr == NULL)
		return *vp = ctx->data;

	memcpy(&ctx->data[ctx->length], ptr, len);
	ctx->length = nl;
	ctx->data[nl] = '\0';
	return *vp = ctx->data;
}

EXPORT_SYMBOL hxmc_t *HXmc_strpcat(hxmc_t **vp, const char *s)
{
	/* Prepend string @s to @*vp */
	if (s == NULL)
		return *vp;
	return HXmc_memins(vp, 0, s, strlen(s));
}

EXPORT_SYMBOL hxmc_t *HXmc_mempcat(hxmc_t **vp, const void *ptr, size_t len)
{
	/* Prepend memory @ptr (of length @len) to @*vp */
	return HXmc_memins(vp, 0, ptr, len);
}

EXPORT_SYMBOL hxmc_t *HXmc_strins(hxmc_t **vp, size_t pos, const char *s)
{
	if (s == NULL)
		return *vp;
	return HXmc_memins(vp, pos, s, strlen(s));
}

/*
 * We naturally do not support negative positions like some
 * scripting languages do, hence @pos is unsigned.
 */

EXPORT_SYMBOL hxmc_t *HXmc_memins(hxmc_t **vp, size_t pos, const void *ptr,
    size_t len)
{
	struct memcont *ctx = HXmc_base(*vp);
	size_t nl = ctx->length + len;

	HXmc_check(ctx);
	if (ctx->alloc < nl) {
		ctx = realloc(ctx, __HXmc_request(nl));
		if (ctx == NULL)
			return NULL;
		ctx->alloc = nl;
	}
	if (ptr == NULL)
		return *vp = ctx->data;

	memmove(&ctx->data[pos + len], &ctx->data[pos], ctx->length - pos);
	memcpy(&ctx->data[pos], ptr, len);
	ctx->length += len;
	ctx->data[ctx->length] = '\0';
	return *vp = ctx->data;
}

EXPORT_SYMBOL hxmc_t *HXmc_memdel(hxmc_t *vp, size_t pos, size_t len)
{
	struct memcont *ctx = HXmc_base(vp);
	HXmc_check(ctx);

	if (pos + len > ctx->length)
		len = ctx->length - pos;

	memmove(&ctx->data[pos], &ctx->data[pos + len],
	        ctx->length - pos - len);
	ctx->length -= len;
	ctx->data[ctx->length] = '\0';
	return ctx->data;
}

EXPORT_SYMBOL void HXmc_free(hxmc_t *vp)
{
	struct memcont *ctx;
	if (vp == NULL)
		return;
	ctx = HXmc_base(vp);
	HXmc_check(ctx);
	free(ctx);
}

EXPORT_SYMBOL void HXmc_zvecfree(hxmc_t **args)
{
	hxmc_t **travp;
	for (travp = args; *travp != NULL; ++travp)
		HXmc_free(*travp);
	free(args);
}
