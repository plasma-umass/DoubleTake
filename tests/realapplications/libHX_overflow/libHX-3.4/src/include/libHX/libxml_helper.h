#ifndef _LIBHX_LIBXML_HELPER_H
#define _LIBHX_LIBXML_HELPER_H 1

#ifdef __cplusplus
#	include <cstring>
#else
#	include <string.h>
#endif
#include <libHX/defs.h>
#include <libxml/parser.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int xml_strcmp(const xmlChar *a, const char *b)
{
#ifdef __cplusplus
	return strcmp(signed_cast<const char *>(a), b);
#else
	return strcmp(signed_cast(const char *, a), b);
#endif
}

static inline int xml_strcasecmp(const xmlChar *a, const char *b)
{
#ifdef __cplusplus
	return strcasecmp(signed_cast<const char *>(a), b);
#else
	return strcasecmp(signed_cast(const char *, a), b);
#endif
}

static inline char *xml_getprop(xmlNode *node, const char *attr)
{
#ifdef __cplusplus
	return signed_cast<char *>(xmlGetProp(node,
	       signed_cast<const xmlChar *>(attr)));
#else
	return signed_cast(char *, xmlGetProp(node,
	       signed_cast(const xmlChar *, attr)));
#endif
}

static inline xmlAttr *xml_newprop(xmlNode *node, const char *name,
    const char *value)
{
#ifdef __cplusplus
	return xmlNewProp(node, signed_cast<const xmlChar *>(name),
	       signed_cast<const xmlChar *>(value));
#else
	return xmlNewProp(node, signed_cast(const xmlChar *, name),
	       signed_cast(const xmlChar *, value));
#endif
}

static inline xmlNode *xml_newnode(xmlNs *ns, const char *name)
{
#ifdef __cplusplus
	return xmlNewNode(ns, signed_cast<const xmlChar *>(name));
#else
	return xmlNewNode(ns, signed_cast(const xmlChar *, name));
#endif
}

static inline xmlAttr *xml_setprop(xmlNode *node, const char *name,
    const char *value)
{
#ifdef __cplusplus
	return xmlSetProp(node, signed_cast<const xmlChar *>(name),
	       signed_cast<const xmlChar *>(value));
#else
	return xmlSetProp(node, signed_cast(const xmlChar *, name),
	       signed_cast(const xmlChar *, value));
#endif
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LIBHX_LIBXML_HELPER_H */
