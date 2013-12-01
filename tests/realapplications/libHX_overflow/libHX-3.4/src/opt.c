/*
 *	libHX/opt.c
 *	Copyright © Jan Engelhardt <jengelh [at] medozas de>, 2002 - 2009
 *
 *	This file is part of libHX. libHX is free software; you can
 *	redistribute it and/or modify it under the terms of the GNU
 *	Lesser General Public License as published by the Free Software
 *	Foundation; either version 2.1 or 3 of the License.
 */
#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libHX/ctype_helper.h>
#include <libHX/deque.h>
#include <libHX/map.h>
#include <libHX/misc.h>
#include <libHX/option.h>
#include <libHX/string.h>
#include "internal.h"

/* Definitions */
#define CALL_CB \
	if (opt->cb != NULL) \
		opt->cb(cbi);
#define NTYPE_S(con, tpx) NTYPE((con), tpx, strtol)
#define NTYPE_U(con, tpx) NTYPE((con), tpx, strtoul)

#define NTYPE(con, tpx, func) case (con): { \
	tpx *p, v = (func)(cbi->data, NULL, 0); \
	if ((p = opt->ptr) != NULL) { \
		if (opt->type & HXOPT_NOT) \
			v = ~v; \
		switch (opt->type & HXOPT_LOPMASK2) { \
			case 0:         *p  = v; break; \
			case HXOPT_OR:  *p |= v; break; \
			case HXOPT_AND: *p &= v; break; \
			case HXOPT_XOR: *p ^= v; break; \
			default: \
				fprintf(stderr, "libHX-opt: illegal " \
				        "combination of logical op mask\n"); \
				break; \
		} \
	} \
	cbi->data_long = v; \
	CALL_CB; \
	break; \
}

#define SCREEN_WIDTH 80 /* fine, popt also has it hardcoded */

enum {
	E_SUCCESS = 0,
	E_LONG_UNKNOWN,
	E_LONG_TAKESVOID,
	E_LONG_MISSING,
	E_SHORT_UNKNOWN,
	E_SHORT_MISSING,

	S_NORMAL = 0,
	S_SHORT,
	S_TWOLONG,
	S_LONG,
	S_TERMINATED,

	W_NONE    = 0,
	W_SPACE   = 1 << 0,
	W_BRACKET = 1 << 1,
	W_ALT     = 1 << 2,
	W_EQUAL   = 1 << 3,

	HXOPT_LOPMASK2 = HXOPT_OR | HXOPT_AND | HXOPT_XOR,
	HXOPT_LOPMASK  = HXOPT_LOPMASK2 | HXOPT_NOT,
	HXOPT_TYPEMASK = 0x1F, /* 5 bits */
};

static void do_assign(struct HXoptcb *cbi)
{
	const struct HXoption *opt = cbi->current;

	switch (opt->type & HXOPT_TYPEMASK) {
	case HXTYPE_NONE: {
		int *p;
		if ((p = opt->ptr) != NULL) {
			p = opt->ptr;
			if (opt->type & HXOPT_INC)      ++*p;
			else if (opt->type & HXOPT_DEC) --*p;
			else                            *p = 1;
		}
		cbi->data_long = 1;
		CALL_CB;
		break;
	}
	case HXTYPE_VAL:
		*static_cast(int *, opt->ptr) = cbi->data_long = opt->val;
		CALL_CB;
		break;
	case HXTYPE_SVAL:
		*reinterpret_cast(const char **, opt->ptr) =
			cbi->data = opt->sval;
		CALL_CB;
		break;
	case HXTYPE_BOOL: {
		int *p;
		if ((p = opt->ptr) != NULL)
			*p = strcasecmp(cbi->data, "yes") == 0 ||
			     strcasecmp(cbi->data, "on") == 0 ||
			     strcasecmp(cbi->data, "true") == 0 ||
			     (HX_isdigit(*cbi->data) &&
			     strtoul(cbi->data, NULL, 0) != 0);
		break;
	}
	case HXTYPE_BYTE:
		*static_cast(unsigned char *, opt->ptr) = *cbi->data;
		CALL_CB;
		break;

	NTYPE_U(HXTYPE_UCHAR,  unsigned char)
	NTYPE_S(HXTYPE_CHAR,   char)
	NTYPE_U(HXTYPE_USHORT, unsigned short);
	NTYPE_S(HXTYPE_SHORT,  short);
	NTYPE_U(HXTYPE_UINT,   unsigned int);
	NTYPE_S(HXTYPE_INT,    int);
	NTYPE_U(HXTYPE_ULONG,  unsigned long);
	NTYPE_S(HXTYPE_LONG,   long);
	NTYPE_U(HXTYPE_UINT8,  uint8_t);
	NTYPE_S(HXTYPE_INT8,   int8_t);
	NTYPE_U(HXTYPE_UINT16, uint16_t);
	NTYPE_S(HXTYPE_INT16,  int16_t);
	NTYPE_U(HXTYPE_UINT32, uint32_t);
	NTYPE_S(HXTYPE_INT32,  int32_t);
#ifndef _MSC_VER
	NTYPE(HXTYPE_ULLONG,   unsigned long long, strtoull);
	NTYPE(HXTYPE_LLONG,    long long, strtoll);
	NTYPE(HXTYPE_UINT64,   uint64_t, strtoull);
	NTYPE(HXTYPE_INT64,    int64_t, strtoll);
#endif
	case HXTYPE_FLOAT:
		cbi->data_dbl = strtod(cbi->data, NULL);
		if (opt->ptr != NULL)
			*static_cast(float *, opt->ptr) = cbi->data_dbl;
		CALL_CB;
		break;
	case HXTYPE_DOUBLE:
		cbi->data_dbl = strtod(cbi->data, NULL);
		if (opt->ptr != NULL)
			*static_cast(double *, opt->ptr) = cbi->data_dbl;
		CALL_CB;
		break;
	case HXTYPE_STRING:
		if (opt->ptr != NULL)
			*static_cast(char **, opt->ptr) = HX_strdup(cbi->data);
		CALL_CB;
		break;
	case HXTYPE_STRDQ:
		HXdeque_push(opt->ptr, HX_strdup(cbi->data));
		CALL_CB;
		break;
	case HXTYPE_MCSTR:
		if (opt->ptr != NULL)
			HXmc_strcpy(opt->ptr, cbi->data);
		CALL_CB;
		break;
	default:
		fprintf(stderr, "libHX-opt: illegal type %d\n",
		        opt->type & HXOPT_TYPEMASK);
		break;
	} /* switch */
}

static inline const struct HXoption *lookup_short(const struct HXoption *table,
    char opt)
{
	for (; table->ln != NULL || table->sh != '\0'; ++table)
		if (table->sh == opt)
			return table;
	return NULL;
}

static inline const struct HXoption *lookup_long(const struct HXoption *table,
    const char *key)
{
	for (; table->ln != NULL || table->sh != '\0'; ++table)
		if (table->ln != NULL && strcmp(table->ln, key) == 0)
			return table;
	return NULL;
}

static inline bool takes_void(unsigned int t)
{
	t &= HXOPT_TYPEMASK;
	return t == HXTYPE_NONE || t == HXTYPE_VAL || t == HXTYPE_SVAL;
}

static void opt_to_text(const struct HXoption *opt, char *buf, size_t len,
    unsigned int flags)
{
	const char *alt, *htyp = (opt->htyp != NULL) ? opt->htyp : "ARG";
	size_t i = 0;
	char equ;

	if (flags & W_SPACE)   buf[i++] = ' ';
	if (flags & W_BRACKET) buf[i++] = '['; /* ] */
	if (flags & W_ALT) {
		alt = "|";
		equ = (flags & W_EQUAL) ? '=' : ' ';
	} else {
		alt = ", ";
		equ = '=';
	}

	if (opt->ln == NULL) {
		buf[i++] = '-';
		buf[i++] = opt->sh;
		if (!takes_void(opt->type))
			i += snprintf(buf + i, len - i, " %s", htyp);
	} else {
		if (opt->sh == '\0') {
			if (takes_void(opt->type))
				i += snprintf(buf + i, len - i,
				     "--%s", opt->ln);
			else
				i += snprintf(buf + i, len - i,
				     "--%s=%s", opt->ln, htyp);
		} else {
			if (takes_void(opt->type))
				i += snprintf(buf + i, len - i, "-%c%s--%s",
				     opt->sh, alt, opt->ln);
			else
				i += snprintf(buf + i, len - i, "-%c%s--%s%c%s",
				     opt->sh, alt, opt->ln, equ, htyp);
		}
	}

	if (flags & W_BRACKET)
		buf[i++] = ']';
	buf[i] = '\0';
}

static void print_indent(const char *msg, unsigned int ind, FILE *fp)
{
	size_t rest = SCREEN_WIDTH - ind;
	char *p;

	while (true) {
		if (strlen(msg) < rest) {
			fprintf(fp, "%s", msg);
			break;
		}
		if ((p = HX_strbchr(msg, msg + rest, ' ')) == NULL) {
			fprintf(fp, "%s", msg);
			break;
		}
		fprintf(fp, "%.*s\n%*s", static_cast(unsigned int, p - msg),
		        msg, ind, "");
		msg  = p + 1;
		rest = SCREEN_WIDTH - ind;
	}
	fprintf(fp, "\n");
}

/**
 * HXparse_deshell_int - shell-style argument unescape
 * @o:		input/output string
 * @end:	terminating characters
 *
 * Unescapes a quoted argument, in-place.
 * Returns a pointer to one position after the termination character.
 */
static char *HXparse_dequote_int(char *o, const char *end)
{
	char *i, quot = '\0';
	for (i = o; *i != '\0'; ) {
		if (quot == '\0') {
			switch (*i) {
				case '"':
				case '\'':
					quot = *i++;
					continue;
				case '\\':
					if (*++i != '\0')
						*o++ = *i++;
					continue;
			}
			if (end != NULL && strchr(end, *i) != NULL) {
				*o = '\0';
				return i + 1;
			}
			*o++ = *i++;
			continue;
		}
		if (*i == quot) {
			quot = 0;
			++i;
			continue;
		} else if (*i == '\\') {
			if (*++i != '\0')
				*o++ = *i++;
			continue;
		}
		*o++ = *i++;
	}
	*o = '\0';
	return NULL;
}

/**
 * HXparse_dequote_fmt
 * @s:		Input string
 * @end:	Terminating characters. May be %NULL.
 * @pptr:	Return pointer
 *
 * Dequote a string @s until @end, and return an allocated string that will
 * contain the result, or %NULL on error. @*pptr will then point to the
 * terminating character.
 * Nested %() are honored.
 *
 * (This function is used from format.c. It is here in opt.c to call
 * HXparse_dequote_int.)
 */
hxmc_t *HXparse_dequote_fmt(const char *s, const char *end, const char **pptr)
{
	unsigned int level = 0; /* nesting */
	const char *i;
	char quot = '\0';
	hxmc_t *tmp;

	/* Search for end */
	for (i = s; *i != '\0'; ) {
		if (quot == '\0') {
			switch (*i) {
				case '"':
				case '\'':
					quot = *i++;
					continue;
				case '\\':
					if (i[1] != '\0')
						++i;
					continue;
			}
			if (i[0] == '%' && i[1] == '(' /* ) */) {
				++level;
				i += 2;
				continue;
			}
			if (level == 0 && end != NULL &&
			    strchr(end, *i) != NULL)
				break;
			if (i[0] == /* ( */ ')' && level > 0)
				--level;
			++i;
			continue;
		}
		if (*i == quot) {
			quot = 0;
			++i;
			continue;
		} else if (*i == '\\') {
			if (*++i != '\0')
				++i;
			continue;
		}
		++i;
	}

	if (pptr != NULL)
		*pptr = i;
	tmp = HXmc_meminit(s, i - s);
	if (tmp == NULL)
		return NULL;
	HXparse_dequote_int(tmp, NULL);
	return tmp;
}

EXPORT_SYMBOL int HX_getopt(const struct HXoption *table, int *argc,
    const char ***argv, unsigned int flags)
{
	const char **opt = *argv, *value = NULL, *shstr = NULL;
	struct HXdeque *remaining = HXdeque_init();
	unsigned int state = S_NORMAL;
	int ret = E_SUCCESS;
	struct HXoptcb cbi;
	char *key = NULL;
	unsigned int argk;

	memset(&cbi, 0, sizeof(cbi));
	cbi.arg0  = **argv;
	cbi.table = table;

	HXdeque_push(remaining, HX_strdup(*opt++)); /* put argv[0] back */

	while (true) {
		const char *cur = *opt;

		if (state == S_TWOLONG) {
			if ((cbi.current = lookup_long(table, key)) == NULL) {
				if (flags & HXOPT_PTHRU) {
					HXdeque_push(remaining, HX_strdup(key));
					++opt;
					state = S_NORMAL;
					continue;
				}
				ret = E_LONG_UNKNOWN;
				break;
			}

			cbi.match_ln = key;
			cbi.match_sh = '\0';

			if (takes_void(cbi.current->type)) {
				cbi.data = NULL;
			} else if (cbi.current->type & HXOPT_OPTIONAL) {
				/*
				 * Rule: take arg if next thing is not-null,
				 * not-option.
				 */
				if (cur == NULL || *cur != '-' ||
				    (cur[0] == '-' && cur[1] == '\0')) {
					/* --file -, --file bla */
					cbi.data = cur;
					cur      = *opt++;
				} else {
					/*
					 * --file --another --file --
					 * endofoptions
					 */
					cbi.data = NULL;
				}
			} else {
				if (cur == NULL) {
					ret = E_LONG_MISSING;
					break;
				}
				cbi.data = cur;
				cur      = *++opt;
			}

			do_assign(&cbi);
			free(key);
			key   = NULL;
			state = S_NORMAL;
			/* fallthrough */
		}

		if (state == S_LONG) {
			bool got_value = (strchr(cur, '=') != NULL);

			if ((cbi.current = lookup_long(table, key)) == NULL) {
				if (flags & HXOPT_PTHRU) {
					HXdeque_push(remaining,
					             HX_strdup(*opt++));
					state = S_NORMAL;
					continue;
				}
				ret = E_LONG_UNKNOWN;
				break;
			}

			if (takes_void(cbi.current->type) && got_value) {
				ret = E_LONG_TAKESVOID;
				break;
			} else if (!takes_void(cbi.current->type) &&
			    !got_value) {
				ret = E_LONG_MISSING;
				break;
			}

			cbi.match_ln = key;
			cbi.match_sh = '\0';
			cbi.data     = value;
			do_assign(&cbi);

			free(key);
			key   = NULL;
			state = S_NORMAL;
			cur   = *++opt;
			/* fallthrough */
		}

		if (state == S_SHORT) {
			if (*shstr == '\0') {
				++opt;
				state = S_NORMAL;
				continue;
			}

			cbi.current = lookup_short(table, *shstr);
			if (cbi.current == NULL) {
				if (flags & HXOPT_PTHRU) {
					char buf[16];
					snprintf(buf, sizeof(buf), "-%s", shstr);
					HXdeque_push(remaining, HX_strdup(buf));
					++opt;
					state = S_NORMAL;
					continue;
				}
				ret = E_SHORT_UNKNOWN;
				break;
			}

			cbi.match_ln = NULL;
			cbi.match_sh = *shstr;

			if (takes_void(cbi.current->type)) {
				/* -A */
				cbi.data = NULL;
				do_assign(&cbi);
				++shstr;
				continue;
			}

			cur = *++opt;
			if (*(shstr + 1) != '\0') {
				/* -Avalue */
				cbi.data = shstr + 1;
				do_assign(&cbi);
				state = S_NORMAL;
				continue;
			}

			if (cbi.current->type & HXOPT_OPTIONAL) {
				if (cur == NULL || *cur != '-' ||
				    (cur[0] == '-' && cur[1] == '\0')) {
					/* --file - --file bla */
					cbi.data = cur;
					cur      = *++opt;
				} else {
					/*
					 * --file --another --file --
					 * endofoptions
					 */
					cbi.data = NULL;
				}
			} else {
				/* -A value */
				if (cur == NULL) {
					ret = E_SHORT_MISSING;
					break;
				}
				cbi.data = cur;
				cur      = *++opt;
			}

			do_assign(&cbi);
			state = S_NORMAL;
			/* fallthrough */
		}

		if (cur == NULL)
			break;

		if (state == S_TERMINATED) {
			HXdeque_push(remaining, HX_strdup(*opt++));
			continue;
		}

		if (state == S_NORMAL) {
			if (cur[0] == '-' && cur[1] == '\0') {
				/*
				 * Note to popt developers: A single dash is
				 * NOT an option!
				 */
				HXdeque_push(remaining, HX_strdup(*opt++));
				continue;
			}
			if (cur[0] == '-' && cur[1] == '-' && cur[2] == '\0') {
				/* double dash */
				state = S_TERMINATED;
				/*
				 * If passthrough is on, "--" must be copied
				 * into @remaining. This is done in the next
				 * round.
				 */
				if (!(flags & HXOPT_PTHRU))
					++opt;
				continue;
			}
			if (cur[0] == '-' && cur[1] == '-') { /* long option */
				char *p;
				key = HX_strdup(cur + 2);
				if ((p = strchr(key, '=')) == NULL) {
					/*
					 * Two argument long option: --long arg
					 */
					state = S_TWOLONG;
					++opt;
					continue;
				}
				/* Single argument long option: --long=arg */
				*p++  = '\0';
				value = p;
				state = S_LONG;
				continue;
			}
			if (cur[0] == '-') {
				/* Short option(s) - one or more(!) */
				state = S_SHORT;
				shstr = cur + 1;
				continue;
			}
			HXdeque_push(remaining, HX_strdup(*opt++));
			continue;
		}

		fprintf(stderr, "libHX-opt: invalid state: %u\n", state);
		state = S_NORMAL;
	}

	if (ret != 0) {
		switch (ret) {
		case E_LONG_UNKNOWN:
			if (!(flags & HXOPT_QUIET))
				fprintf(stderr, "Unknown option: --%s\n", key);
			ret = -HXOPT_ERR_UNKN;
			break;
		case E_LONG_TAKESVOID:
			if (!(flags & HXOPT_QUIET))
				fprintf(stderr, "Option --%s does not take "
				        "any argument\n", key);
			ret = -HXOPT_ERR_VOID;
			break;
		case E_LONG_MISSING:
			if (!(flags & HXOPT_QUIET))
				fprintf(stderr, "Option --%s requires an "
				        "argument\n", key);
			ret = -HXOPT_ERR_MIS;
			break;
		case E_SHORT_UNKNOWN:
			if (!(flags & HXOPT_QUIET))
				fprintf(stderr, "Unknown option: -%c\n",
				        *shstr);
			ret = -HXOPT_ERR_UNKN;
			break;
		case E_SHORT_MISSING:
			if (!(flags & HXOPT_QUIET))
				fprintf(stderr, "Option -%c requires an "
				        "argument\n", *shstr);
			ret = -HXOPT_ERR_MIS;
			break;
		} /* switch */
		free(key);

		if (flags & HXOPT_HELPONERR)
			HX_getopt_help(&cbi, stderr);
		else if (flags & HXOPT_USAGEONERR)
			HX_getopt_usage(&cbi, stderr);

		HXdeque_genocide(remaining);
		return ret;
	}

	if (flags & HXOPT_DESTROY_OLD)
		/*
		 * Only the "true, original" argv is stored on the stack - the
		 * argv that HX_getopt() produces is on the heap, so the
		 * HXOPT_DESTROY_OLD flag should be passed when you use
		 * passthrough chaining, i.e. all but the first call to
		 * HX_getopt() should have this set.
		 */
		HX_zvecfree(const_cast2(char **, *argv));

	*argv = reinterpret_cast(const char **,
	        HXdeque_to_vec(remaining, &argk));
	*argc = argk;
	HXdeque_free(remaining);
	return 1;
}

EXPORT_SYMBOL void HX_getopt_help(const struct HXoptcb *cbi, FILE *nfp)
{
	FILE *fp = (nfp == NULL) ? stderr : nfp;
	const struct HXoption *travp;
	char tmp[84] = {'\0'};
	unsigned int tw = 0;

	HX_getopt_usage(cbi, nfp);

	/* Find maximum indent */
	for (travp = cbi->table; travp->ln != NULL || travp->sh != '\0'; ++travp) {
		size_t tl;

		opt_to_text(travp, tmp, sizeof(tmp), W_EQUAL);
		if ((tl = strlen(tmp)) > tw)
			tw = tl;
	}

	/* Print table */
	for (travp = cbi->table; travp->ln != NULL || travp->sh != '\0'; ++travp) {
		opt_to_text(travp, tmp, sizeof(tmp), W_NONE);
		fprintf(fp, "  %-*s    ", static_cast(int, tw), tmp);
		if (travp->help == NULL)
			fprintf(fp, "\n");
		else
			print_indent(travp->help, tw + 6, fp);
	}
}

EXPORT_SYMBOL void HX_getopt_help_cb(const struct HXoptcb *cbi)
{
	HX_getopt_help(cbi, stdout);
	exit(EXIT_SUCCESS);
}

EXPORT_SYMBOL void HX_getopt_usage(const struct HXoptcb *cbi, FILE *nfp)
{
	size_t wd = sizeof("Usage:") + strlen(HX_basename(cbi->arg0)), tw = 0;
	FILE *fp = (nfp == NULL) ? stderr : nfp;
	const struct HXoption *travp;
	char tmp[84] = {};

	fprintf(fp, "Usage: %s", HX_basename(cbi->arg0));

	/* Short-only flags */
	if (wd + 5 > SCREEN_WIDTH) {
		/* 5 is the minimum size for a new starting option, " [-X]" */
		fprintf(fp, "\n     ");
		wd = 6;
	}
	for (travp = cbi->table; travp->ln != NULL || travp->sh != '\0'; ++travp) {
		if (!(travp->ln == NULL && travp->sh != '\0' &&
		    takes_void(travp->type)))
			continue;
		if (*tmp == '\0') {
			snprintf(tmp, sizeof(tmp), " [-"); /* ] */
			tw = 3;
		}
		tmp[tw++] = travp->sh;
		if (wd + tw + 1 > SCREEN_WIDTH) {
			tmp[tw++] = /* [ */ ']';
			tmp[tw]   = '\0';
			fprintf(fp, "%s\n      ", tmp);
			wd   = 6;
			*tmp = '\0';
		}
	}
	if (*tmp != '\0') {
		tmp[tw++] = ']';
		tmp[tw]   = '\0';
		wd += fprintf(fp, "%s", tmp);
	}

	/* Any other args */
	for (travp = cbi->table; travp->ln != NULL || travp->sh != '\0'; ++travp) {
		if (travp->ln == NULL && travp->sh != '\0' &&
		    takes_void(travp->type))
			continue;

		opt_to_text(travp, tmp, sizeof(tmp),
		            W_SPACE | W_BRACKET | W_ALT);
		if (wd + strlen(tmp) > SCREEN_WIDTH) {
			fprintf(fp, "\n      ");
			wd = 6;
		}
		wd += fprintf(fp, "%s", tmp);
	}

	fprintf(fp, "\n");
}

EXPORT_SYMBOL void HX_getopt_usage_cb(const struct HXoptcb *cbi)
{
	HX_getopt_usage(cbi, stdout);
	exit(EXIT_SUCCESS);
}

static void HX_shconf_break(void *ptr, char *line,
    void (*cb)(void *, const char *, const char *))
{
	char *lp = line, *key, *val;
	HX_chomp(line);

	while (lp != NULL) {
		while (HX_isspace(*lp) || *lp == ';')
			++lp;
		/* Next entry if comment, empty line or no value */
		if (*lp == '#' || *lp == '\0')
			return;
		if (!HX_isalpha(*lp) && *lp != '_')
			/* Variables ought to start with [A-Z_] */
			return;
		key = lp;
		while (HX_isalnum(*lp) || *lp == '_')
			++lp;
		if (*lp != '=')
			/* Variable name contained something not in [A-Z0-9_] */
			return;
		*lp++ = '\0';
		val = lp;

		/* Handle escape codes and quotes, and assign to TAB entry */
		lp = HXparse_dequote_int(val, "\t\n ;");
		(*cb)(ptr, key, val);
	}
}

static void HX_shconf_assign(void *table, const char *key, const char *value)
{
	struct HXoptcb cbi = {.table = table, .match_sh = '\0'};

	if ((cbi.current = lookup_long(table, key)) == NULL)
		return;

	cbi.match_ln = key;
	cbi.data     = value;
	do_assign(&cbi);
}

EXPORT_SYMBOL int HX_shconfig(const char *file, const struct HXoption *table)
{
	hxmc_t *ln = NULL;
	FILE *fp;

	if ((fp = fopen(file, "r")) == NULL)
		return -errno;

	while (HX_getl(&ln, fp) != NULL)
		HX_shconf_break(const_cast(void *,
			static_cast(const void *, table)), ln,
			HX_shconf_assign);

	HXmc_free(ln);
	fclose(fp);
	return 1;
}

static void HX_shconf_assignmp(void *map, const char *key, const char *value)
{
	HXmap_add(map, key, value);
}

EXPORT_SYMBOL struct HXmap *HX_shconfig_map(const char *file)
{
	struct HXmap *map;
	hxmc_t *ln = NULL;
	FILE *fp;

	map = HXmap_init(HXMAPT_DEFAULT, HXMAP_SCKEY | HXMAP_SCDATA);
	if (map == NULL)
		return NULL;

	if ((fp = fopen(file, "r")) == NULL) {
		free(map);
		return NULL;
	}

	while (HX_getl(&ln, fp) != NULL)
		HX_shconf_break(map, ln, HX_shconf_assignmp);

	HXmc_free(ln);
	fclose(fp);
	return map;
}

EXPORT_SYMBOL int HX_shconfig_pv(const char **path, const char *file,
    const struct HXoption *table, unsigned int flags)
{
	char buf[MAXFNLEN];
	int ret = 0;

	for (; *path != NULL; ++path) {
		int v;
		snprintf(buf, sizeof(buf), "%s/%s", *path, file);
		v = HX_shconfig(buf, table);
		if (v > 0) {
			++ret;
			if (flags & SHCONF_ONE)
				break;
		}
	}

	return ret;
}

EXPORT_SYMBOL void HX_shconfig_free(const struct HXoption *table)
{
	for (; table->ln != NULL; ++table) {
		char **ptr = table->ptr;
		if (table->type == HXTYPE_STRING &&
		    ptr != NULL && *ptr != NULL)
			free(*ptr);
	}
}
