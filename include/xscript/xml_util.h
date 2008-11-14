#ifndef _XSCRIPT_XML_UTIL_H_
#define _XSCRIPT_XML_UTIL_H_

#include <sys/time.h>
#include <ctime>
#include <string>
#include <vector>
#include <iosfwd>
#include <stdexcept>

#include <boost/cstdint.hpp>
#include <boost/thread/tss.hpp>
#include <boost/utility.hpp>

#include <xscript/config.h>
#include <xscript/context.h>
#include <xscript/range.h>
#include <xscript/xml.h>
#include <xscript/xml_helpers.h>

#include <libxml/tree.h>

namespace xscript {

class Encoder;

class XmlUtils : private boost::noncopyable {
public:
    XmlUtils();
    virtual ~XmlUtils();

    static void init(const Config *config);

    static void registerReporters();
    static void resetReporter();
    static void throwUnless(bool value);
    static bool hasXMLError();
    static void printXMLError(const std::string& postfix);

    static void reportXsltError(const std::string &error, xmlXPathParserContextPtr ctxt);
    static void reportXsltError(const std::string &error, const Context *ctx);

    static xmlParserInputPtr entityResolver(const char *url, const char *id, xmlParserCtxtPtr ctxt);

    static std::string escape(const Range &value);
    template<typename Cont> static std::string escape(const Cont &value);

    static std::string sanitize(const Range &value);
    template<typename Cont> static std::string sanitize(const Cont &value);

    template<typename NodePtr> static const char* value(NodePtr node);

    template<typename Visitor> static void visitAttributes(xmlAttrPtr attr, Visitor visitor);

    static inline const char* attrValue(xmlNodePtr node, const char *name) {
        xmlAttrPtr attr = xmlHasProp(node, (const xmlChar*) name);
        return attr ? value(attr) : NULL;
    }

    static bool xpathExists(xmlDocPtr doc, const std::string &path);
    static std::string xpathValue(xmlDocPtr doc, const std::string &path, const std::string &defval = "");

public:
    static const char * const XSCRIPT_NAMESPACE;

private:
    static xmlExternalEntityLoader default_loader_;
};

class XmlInfoCollector {
public:
    XmlInfoCollector();

    static void ready(bool flag);
    static Xml::TimeMapType* getModifiedInfo();

private:
    static boost::thread_specific_ptr<Xml::TimeMapType> modified_info_;
};

template<typename Cont> inline std::string
XmlUtils::escape(const Cont &value) {
    return escape(createRange(value));
}

template<typename Cont> inline std::string
XmlUtils::sanitize(const Cont &value) {
    return sanitize(createRange(value));
}

template <typename NodePtr> inline const char*
XmlUtils::value(NodePtr node) {
    xmlNodePtr child = node->children;
    if (child && xmlNodeIsText(child) && child->content) {
        return (const char*) child->content;
    }
    return NULL;
}

template<typename Visitor> inline void
XmlUtils::visitAttributes(xmlAttrPtr attr, Visitor visitor) {
    std::size_t len = strlen(XSCRIPT_NAMESPACE) + 1;
    while (attr) {
        xmlNsPtr ns = attr->ns;
        if (NULL == ns || xmlStrncmp(ns->href, (const xmlChar*) XSCRIPT_NAMESPACE, len) == 0) {
            const char *name = (const char*) attr->name, *val = value(attr);
            if (name && val) {
                visitor(name, val);
            }
        }
        attr = attr->next;
    }
}

} // namespace xscript

#endif // _XSCRIPT_XML_UTIL_H_
