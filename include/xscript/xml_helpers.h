#ifndef _XSCRIPT_XML_HELPERS_H_
#define _XSCRIPT_XML_HELPERS_H_

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxslt/transform.h>
#include <libxslt/xsltInternals.h>

#include <xscript/helper.h>

namespace xscript
{

struct XmlDocTraits : public TypeTraits<xmlDocPtr>
{
	static inline void clean(xmlDocPtr doc) {
		xmlFreeDoc(doc);
	}
};

struct XmlCharTraits : public TypeTraits<xmlChar*>
{
	static inline void clean(xmlChar *value) {
		xmlFree(value);
	}
};

struct XmlNodeTraits : public TypeTraits<xmlNodePtr>
{
	static inline void clean(xmlNodePtr node) {
		xmlFreeNode(node);
	}
};

struct XmlXPathObjectTraits : public TypeTraits<xmlXPathObjectPtr>
{
	static inline void clean(xmlXPathObjectPtr obj) {
		xmlXPathFreeObject(obj);
	}
};

struct XmlXPathContextTraits : public TypeTraits<xmlXPathContextPtr>
{
	static inline void clean(xmlXPathContextPtr ctx) {
		xmlXPathFreeContext(ctx);
	}
};

struct XsltStylesheetTraits : public TypeTraits<xsltStylesheetPtr>
{
	static inline void clean(xsltStylesheetPtr sh) {
		xsltFreeStylesheet(sh);
	}
};

struct XsltTransformContextTraits : public TypeTraits<xsltTransformContextPtr>
{
	static inline void clean(xsltTransformContextPtr ctx) {
		xsltFreeTransformContext(ctx);
	}
};

typedef Helper<xmlDocPtr, XmlDocTraits> XmlDocHelper;
typedef Helper<xmlChar*, XmlCharTraits> XmlCharHelper;
typedef Helper<xmlNodePtr, XmlNodeTraits> XmlNodeHelper;
typedef Helper<xmlXPathObjectPtr, XmlXPathObjectTraits> XmlXPathObjectHelper;
typedef Helper<xmlXPathContextPtr, XmlXPathContextTraits> XmlXPathContextHelper;

typedef Helper<xsltStylesheetPtr, XsltStylesheetTraits> XsltStylesheetHelper;
typedef Helper<xsltTransformContextPtr, XsltTransformContextTraits> XsltTransformContextHelper;

} // namespace xscript

#endif // _XSCRIPT_XML_HELPERS_H_
