#include "settings.h"

#include "xscript/typed_map.h"
#include "xscript/xml_util.h"

#include "state_param_node.h"
#include "xml_node.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

StateParamNode::StateParamNode(xmlNodePtr parent, const char *name)
        : parent_(parent)
        , name_(name)
        , is_valid_name_(checkName(name)) {
}

StateParamNode::~StateParamNode() {
}

void
StateParamNode::createSubNode(const char *val) const {
    XmlChildNode child_param(parent_, "param", val);
    child_param.setProperty("name", name_);

    if (is_valid_name_) {
        XmlChildNode(parent_, name_, val);
    }
}

void
StateParamNode::createSubNode(const TypedValue &value) const {
    XmlTypedVisitor visitor;
    value.visit(&visitor);
    XmlNodeHelper result = visitor.result();
    xmlNewProp(result.get(), (const xmlChar*) "name", (const xmlChar*) name_);
    xmlAddChild(parent_, result.release());
}

void
StateParamNode::createSubNodes(const std::vector<std::string> &v) const {
    for (std::vector<std::string>::const_iterator i = v.begin(), end = v.end(); i != end; ++i) {
        createSubNode(i->c_str());
    }
}

bool
StateParamNode::checkName(const char *name) {
    if (!*name) {
        return false;
    }

    if (!isalpha(*name) && *name != '_') {
        return false;
    }
    
    while(*(++name)) {
        if (!isalnum(*name) && *name != '_' && *name != '.' && *name != '-') {
            return false;
        }
    }
    return true;
}

} // namespace xscript
