#ifndef _XSCRIPT_XML_UTIL_H_
#define _XSCRIPT_XML_UTIL_H_

#include <map>
#include <string>

#include <boost/cstdint.hpp>
#include <boost/utility.hpp>

#include <xscript/range.h>
#include <xscript/string_utils.h>
#include <xscript/typed_map.h>
#include <xscript/xml.h>
#include <xscript/xml_helpers.h>

#include <libxml/tree.h>
#include <libxslt/transform.h>

namespace xscript {

class Block;
class Config;
class Encoder;

class XmlUtils : private boost::noncopyable {
public:
    XmlUtils();
    virtual ~XmlUtils();

    static void init(const Config *config);

    static void registerReporters();
    static void resetReporter();
    static void throwUnless(bool value);
    static void throwUnless(bool value, const char *attr, const char *attr_value);
    static bool hasXMLError();
    static std::string getXMLError();
    static void printXMLError(const std::string& postfix);

    static void reportXsltError(const std::string &error, xmlXPathParserContextPtr ctxt);
    static void reportXsltError(const std::string &error, xsltTransformContextPtr tctx);
    static void reportXsltError(const std::string &error, xsltTransformContextPtr tctx, bool strong_check);

    static xmlParserInputPtr entityResolver(const char *url, const char *id, xmlParserCtxtPtr ctxt);

    static std::string escape(const Range &value);
    template<typename Cont> static std::string escape(const Cont &value);

    static std::string sanitize(const Range &value, const std::string &base_url, int line_limit);
    template<typename Cont> static std::string sanitize(const Cont &value, const std::string &base_url, int line_limit);

    template<typename NodePtr> static const char* value(NodePtr node);
    template<typename NodePtr> static const char* cdataValue(NodePtr node);

    template<typename Visitor> static void visitAttributes(xmlAttrPtr attr, Visitor visitor);

    static inline const char* attrValue(xmlNodePtr node, const char *name) {
        xmlAttrPtr attr = xmlHasProp(node, (const xmlChar*) name);
        return attr ? value(attr) : NULL;
    }

    static bool xpathExists(xmlDocPtr doc, const std::string &path);
    static std::string xpathValue(xmlDocPtr doc, const std::string &path,
            const std::string &defval = StringUtils::EMPTY_STRING);
    static bool xpathNsExists(xmlDocPtr doc, const std::string &path,
        const std::map<std::string, std::string> &ns);
    static std::string xpathNsValue(xmlDocPtr doc, const std::string &path,
        const std::map<std::string, std::string> &ns,
            const std::string &defval = StringUtils::EMPTY_STRING);
    
    static xmlDocPtr fakeXml();

    static bool validate(const std::string &data);
    
    static std::string getUniqueNodeId(xmlNodePtr node);
    
    static int xmlVersionNumber();
    static int xsltVersionNumber();
    static int exsltVersionNumber();

    static const char* xmlVersion();
    static const char* xsltVersion();
    static const char* exsltVersion();

    static void regiserNsList(xmlXPathContextPtr ctx, const std::map<std::string, std::string> &ns);

    static const char * const XSCRIPT_NAMESPACE;
};

class XmlTypedVisitor : public TypedValueVisitor {
public:
    XmlTypedVisitor(const std::string &name);
    virtual ~XmlTypedVisitor();
    virtual void visitString(const std::string &value);
    virtual void visitArray(const std::vector<std::string> &value);
    virtual void visitMap(const std::map<std::string, std::string> &value);
    void reset();
    XmlNodeSetHelper result() const;
protected:
    void setResult(XmlNodeSetHelper result);
private:
    std::string name_;
    XmlNodeSetHelper result_;
};

class XmlInfoCollector {
public:
    XmlInfoCollector();

    typedef std::map<std::string, std::string> ErrorMapType;
    
    static void ready(bool flag);
    static Xml::TimeMapType* getModifiedInfo();
    static ErrorMapType* getErrorInfo();
    static std::string getError();
    
    class Starter {
    public:
        Starter();
        ~Starter();
    };
};

template<typename Cont> inline std::string
XmlUtils::escape(const Cont &value) {
    return escape(createRange(value));
}

template<typename Cont> inline std::string
XmlUtils::sanitize(const Cont &value, const std::string &base_url, int line_limit) {
    return sanitize(createRange(value), base_url, line_limit);
}

template <typename NodePtr> inline const char*
XmlUtils::value(NodePtr node) {
    xmlNodePtr child = node->children;
    if (child && xmlNodeIsText(child) && child->content) {
        return (const char*) child->content;
    }
    return NULL;
}

template <typename NodePtr> inline const char*
XmlUtils::cdataValue(NodePtr node) {
    const char* value = NULL;
    for (xmlNodePtr ptr = node->children; NULL != ptr; ptr = ptr->next) {
        if (XML_CDATA_SECTION_NODE == ptr->type) {
            value = (const char*)ptr->content;
            break;
        }
    }
    return value;
}

template<typename Visitor> inline void
XmlUtils::visitAttributes(xmlAttrPtr attr, Visitor visitor) {
    for ( ; attr; attr = attr->next ) {
        if (!attr->name) {
    	    continue;
        }
        xmlNsPtr ns = attr->ns;
        if (NULL == ns || !strcmp((const char*) ns->href, XSCRIPT_NAMESPACE)) {
            const char *val = value(attr);
            if (val) {
                const char *name = (const char*) attr->name;
                visitor(name, val);
            }
        }
    }
}

} // namespace xscript

#endif // _XSCRIPT_XML_UTIL_H_
