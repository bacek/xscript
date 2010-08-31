#include "settings.h"

#include <libxml/xmlstring.h>
#include <libxml/xpathInternals.h>

#include "xml_node.h"

#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

XmlNodeCommon::XmlNodeCommon()
        : node_(NULL) {
}

XmlNodeCommon::~XmlNodeCommon() {
}

xmlNodePtr XmlNodeCommon::getNode() const {
    return node_;
}

void
XmlNodeCommon::setContent(const char* val) {
    xmlNodeSetContent(node_, (const xmlChar*) val);
}

void
XmlNodeCommon::setProperty(const char* name, const char* val) {
    xmlNewProp(node_, (const xmlChar*) name, (const xmlChar*) val);
}

void
XmlNodeCommon::setType(const char* type_str) {
    setProperty("type", type_str);
}



XmlNode::XmlNode(const char* name)
        : XmlNodeCommon() {
    node_ = xmlNewNode(NULL, (const xmlChar*) name);
    XmlUtils::throwUnless(NULL != node_);
}

XmlNode::~XmlNode() {
    if (node_ != NULL) {
        xmlFreeNode(node_);
    }
}

xmlNodePtr
XmlNode::releaseNode() {
    xmlNodePtr ret_node = node_;
    node_ = NULL;
    return ret_node;
}

XmlChildNode::XmlChildNode(xmlNodePtr parent, const char* name, const char* val)
        : XmlNodeCommon() {
    node_ = xmlNewTextChild(parent, NULL, (const xmlChar*) name, (const xmlChar*) val);
    XmlUtils::throwUnless(NULL != node_);
}


} // namespace xscript
