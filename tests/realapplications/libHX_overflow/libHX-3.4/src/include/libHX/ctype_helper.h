#ifndef _LIBHX_CTYPE_H
#define _LIBHX_CTYPE_H 1

#ifdef __cplusplus
#	include <cctype>
#else
#	include <ctype.h>
#	include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ctype.h workarounds. The is*() functions takes an int, but people
 * commonly pass in a char. Because char can technically be signed, the
 * value would be sign-extended during promotion to the int type which the
 * ctype family functions take.
 *
 * The HX_ ctype related functions therefore explicitly take an unsigned char,
 * and thus knowingly make it impossible to pass in stdio's "EOF" (-1).
 * [When was the last time you did a isalpha(EOF) anyway?]
 *
 * Because, again, this all works due to implicit type conversion, these
 * wrappers look rather plain. Oh, also note we are returning the much
 * more modern "bool".
 *
 * And not all ctype functions are provided - no need so far, and I do not
 * want to clutter it before needing it.
 */
static inline bool HX_isalnum(unsigned char c)
{
	return isalnum(c);
}

static inline bool HX_isalpha(unsigned char c)
{
	return isalpha(c);
}

static inline bool HX_isdigit(unsigned char c)
{
	return isdigit(c);
}

static inline bool HX_islower(unsigned char c)
{
	return islower(c);
}

static inline bool HX_isprint(unsigned char c)
{
	return isprint(c);
}

static inline bool HX_isspace(unsigned char c)
{
	return isspace(c);
}

static inline bool HX_isupper(unsigned char c)
{
	return isupper(c);
}

static inline bool HX_isxdigit(unsigned char c)
{
	return isxdigit(c);
}

static inline unsigned char HX_tolower(unsigned char c)
{
	return tolower(c);
}

static inline unsigned char HX_toupper(unsigned char c)
{
	return toupper(c);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LIBHX_CTYPE_H */
