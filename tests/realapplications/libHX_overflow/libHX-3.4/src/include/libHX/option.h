#ifndef _LIBHX_OPTION_H
#define _LIBHX_OPTION_H 1

#ifdef __cplusplus
#	include <cstdio>
#else
#	include <stdio.h>
#endif
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __libhx_internal_hxmc_t_defined
#define __libhx_internal_hxmc_t_defined 1
typedef char hxmc_t;
#endif

struct HXformat_map;
struct HXoption;

/*
 *	FORMAT.C
 */
extern struct HXformat_map *HXformat_init(void);
extern void HXformat_free(struct HXformat_map *);
extern int HXformat_add(struct HXformat_map *, const char *, const void *,
	unsigned int);
extern int HXformat_aprintf(const struct HXformat_map *,
	hxmc_t **, const char *);
extern int HXformat_sprintf(const struct HXformat_map *,
	char *, size_t, const char *);
extern int HXformat_fprintf(const struct HXformat_map *,
	FILE *, const char *);
extern int HXformat2_aprintf(const struct HXformat_map *,
	hxmc_t **, const char *);
extern int HXformat2_sprintf(const struct HXformat_map *,
	char *, size_t, const char *);
extern int HXformat2_fprintf(const struct HXformat_map *,
	FILE *, const char *);

/*
 *	OPT.C
 */
enum {
	/* .type */
	HXTYPE_NONE = 0,
	/* for opt: set specific integer value */
	HXTYPE_VAL,
	/* for opt: set specific string value */
	HXTYPE_SVAL,
	/*
	 * accept a string "yes", "no", "true", "false" and
	 * put into *(unsigned int*)
	 */
	HXTYPE_BOOL,
	/* read _one byte_ and put it into *(unsigned char *) */
	HXTYPE_BYTE,
	/* read an integer/float (sscanf %d/%o/%x/%f) */
	HXTYPE_UCHAR,
	HXTYPE_CHAR,
	HXTYPE_USHORT,
	HXTYPE_SHORT,
	HXTYPE_UINT,
	HXTYPE_INT,
	HXTYPE_ULONG,
	HXTYPE_LONG,
	HXTYPE_ULLONG,
	HXTYPE_LLONG,
	HXTYPE_FLOAT,
	HXTYPE_DOUBLE,
	/* read string and put into *(const char **) */
	HXTYPE_STRING,
	HXTYPE_STRP, /* (const char **) */
	HXTYPE_STRDQ,
	HXTYPE_UINT8,
	HXTYPE_UINT16,
	HXTYPE_UINT32,
	HXTYPE_UINT64,
	HXTYPE_INT8,
	HXTYPE_INT16,
	HXTYPE_INT32,
	HXTYPE_INT64,
	HXTYPE_MCSTR, /* put into hxmc_t */

	/* .type extra flags */
	/* argument is optional */
	HXOPT_OPTIONAL = 1 << 6,
	/* increase pointed variable */
	HXOPT_INC      = 1 << 7,
	/* decrease pointed variable */
	HXOPT_DEC      = 1 << 8,
	/* negate input first */
	HXOPT_NOT      = 1 << 9,
	/* or pointed variable with input */
	HXOPT_OR       = 1 << 10,
	/* and pointed variable with input */
	HXOPT_AND      = 1 << 11,
	/* xor pointed variable with input */
	HXOPT_XOR      = 1 << 12,
	HXFORMAT_IMMED = 1 << 13,

	/* HX_getopt() flags */
	HXOPT_PTHRU       = 1 << 0,
	HXOPT_DESTROY_OLD = 1 << 1,
	HXOPT_QUIET       = 1 << 2,
	HXOPT_HELPONERR   = 1 << 3,
	HXOPT_USAGEONERR  = 1 << 4,

	/* Return types for HX_getopt() */
	HXOPT_ERR_UNKN = 1,
	HXOPT_ERR_VOID,
	HXOPT_ERR_MIS,

	SHCONF_ONE = 1 << 0, /* only read one configuration file */
};

struct HXoptcb {
	const char *arg0;
	const struct HXoption *table, *current;
	const char *data;
	union {
		double data_dbl;
		long data_long;
	};
	const char *match_ln;
	char match_sh;
};

struct HXoption {
	const char *ln;
	char sh;
	unsigned int type;
	void *ptr, *uptr;
	void (*cb)(const struct HXoptcb *);
	int val;
	const char *sval, *help, *htyp;
};

extern int HX_getopt(const struct HXoption *, int *, const char ***,
	unsigned int);
extern void HX_getopt_help(const struct HXoptcb *, FILE *);
extern void HX_getopt_help_cb(const struct HXoptcb *);
extern void HX_getopt_usage(const struct HXoptcb *, FILE *);
extern void HX_getopt_usage_cb(const struct HXoptcb *);
extern int HX_shconfig(const char *, const struct HXoption *);
extern struct HXmap *HX_shconfig_map(const char *);
extern int HX_shconfig_pv(const char **, const char *,
	const struct HXoption *, unsigned int);
extern void HX_shconfig_free(const struct HXoption *);

#ifdef __cplusplus
} /* extern "C" */
#endif

#ifndef __cplusplus
#	define HXOPT_AUTOHELP \
		{.ln = "help", .sh = '?', .type = HXTYPE_NONE, \
		.cb = HX_getopt_help_cb, .help = "Show this help message"}, \
		{.ln = "usage", .type = HXTYPE_NONE, \
		.cb = HX_getopt_usage_cb, \
		.help = "Display brief usage message"}
#	define HXOPT_TABLEEND {.ln = NULL, .sh = 0}
#else
#	define HXOPT_AUTOHELP \
		{NULL, '?', HXTYPE_NONE, NULL, NULL, HX_getopt_help_cb, \
		0, NULL, "Show this help message"}
#	define HXOPT_TABLEEND {NULL, 0}
#endif

#endif /* _LIBHX_OPTION_H */
