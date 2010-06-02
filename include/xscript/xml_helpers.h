#ifndef _XSCRIPT_XML_HELPERS_H_
#define _XSCRIPT_XML_HELPERS_H_

#include <boost/shared_ptr.hpp>

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxslt/transform.h>
#include <libxslt/xsltInternals.h>

#include "xscript/resource_holder.h"

namespace xscript {

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
inline void ResourceHolderTraits<xmlXPathCompExprPtr>::destroy(xmlXPathCompExprPtr expr) {
    xmlXPathFreeCompExpr(expr);
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
typedef ResourceHolder<xmlNodeSetPtr> XmlNodeSetHelper;
typedef ResourceHolder<xmlXPathObjectPtr> XmlXPathObjectHelper;
typedef ResourceHolder<xmlXPathContextPtr> XmlXPathContextHelper;
typedef ResourceHolder<xmlXPathCompExprPtr> XmlXPathCompExprHelper;
typedef ResourceHolder<xsltStylesheetPtr> XsltStylesheetHelper;
typedef ResourceHolder<xsltTransformContextPtr> XsltTransformContextHelper;

class XmlDocSharedHelper {
public:
    XmlDocSharedHelper() : doc_((xmlDocPtr)NULL, &xmlFreeDoc) {}
    XmlDocSharedHelper(xmlDocPtr doc) : doc_(doc, &xmlFreeDoc) {}
    XmlDocSharedHelper(const XmlDocSharedHelper &doc) : doc_(doc.doc_) {}

    void reset() {
        doc_.reset();
    }

    void reset(xmlDocPtr doc) {
        doc_.reset(doc, &xmlFreeDoc);
    }

    XmlDocSharedHelper& operator=(const XmlDocSharedHelper &doc) {
        doc_ = doc.doc_;
        return *this;
    }

    xmlDoc& operator*() const {
        return *doc_;
    }

    xmlDocPtr operator->() const {
        return doc_.operator->();
    }

    xmlDocPtr get() const {
        return doc_.get();
    }

    bool unique() const {
        return doc_.unique();
    }

    long use_count() const {
        return doc_.use_count();
    }

    void swap(XmlDocSharedHelper &doc) {
        doc_.swap(doc.doc_);
    }
private:
    boost::shared_ptr<xmlDoc> doc_;
};

} // namespace xscript

#endif // _XSCRIPT_XML_HELPERS_H_
