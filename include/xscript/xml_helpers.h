#ifndef _XSCRIPT_XML_HELPERS_H_
#define _XSCRIPT_XML_HELPERS_H_

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxslt/transform.h>
#include <libxslt/xsltInternals.h>

#include "xscript/resource_holder.h"

namespace xscript
{

template<> 
inline void ResourceHolderTraits<xmlDocPtr>::destroy(xmlDocPtr doc) {
    xmlFreeDoc(doc);
}

template<> 
inline void ResourceHolderTraits<xmlChar*>::destroy(xmlChar *value) {
    xmlFree(value);
};

template<> 
inline void ResourceHolderTraits<xmlNodePtr>::destroy(xmlNodePtr node) {
    xmlFreeNode(node);
};

template<> 
inline void ResourceHolderTraits<xmlNodeSetPtr>::destroy(xmlNodeSetPtr node_set) {
	xmlXPathFreeNodeSet(node_set);
};

template<> 
inline void ResourceHolderTraits<xmlXPathObjectPtr>::destroy(xmlXPathObjectPtr obj) {
    xmlXPathFreeObject(obj);
};

template<> 
inline void ResourceHolderTraits<xmlXPathContextPtr>::destroy(xmlXPathContextPtr ctx) {
    xmlXPathFreeContext(ctx);
};

template<>
inline void ResourceHolderTraits<xsltStylesheetPtr>::destroy(xsltStylesheetPtr sh) {
    xsltFreeStylesheet(sh);
};

template<>
inline void ResourceHolderTraits<xsltTransformContextPtr>::destroy(xsltTransformContextPtr ctx) {
    xsltFreeTransformContext(ctx);
};

typedef ResourceHolder<xmlDocPtr> XmlDocHelper;
typedef ResourceHolder<xmlChar*> XmlCharHelper;
typedef ResourceHolder<xmlNodePtr> XmlNodeHelper;
typedef ResourceHolder<xmlNodeSetPtr> xmlNodeSetHelper;
typedef ResourceHolder<xmlXPathObjectPtr> XmlXPathObjectHelper;
typedef ResourceHolder<xmlXPathContextPtr> XmlXPathContextHelper;
typedef ResourceHolder<xsltStylesheetPtr> XsltStylesheetHelper;
typedef ResourceHolder<xsltTransformContextPtr> XsltTransformContextHelper;


} // namespace xscript

#endif // _XSCRIPT_XML_HELPERS_H_
