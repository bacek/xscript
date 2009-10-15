#include "settings.h"

#include "xscript/block.h"
#include "xscript/context.h"
#include "xscript/control_extension.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

static const std::string CONTROL_KEY = "CONTROL_KEY";

ControlExtension::ControlExtension() {
}

ControlExtension::~ControlExtension() {
    constructors().clear();
}

const char*
ControlExtension::name() const {
    return "control";
}

const char*
ControlExtension::nsref() const {
    return "http://www.yandex.ru/xscript";
}

void
ControlExtension::initContext(Context *) {
}

void
ControlExtension::stopContext(Context *) {
}

void
ControlExtension::destroyContext(Context *) {
}

std::auto_ptr<Block>
ControlExtension::createBlock(Xml *owner, xmlNodePtr orig) {
    std::string method;
    xmlNodePtr node = orig->children;
    while (node) {
        if (!node->name) {
            continue;
        }
        else if (XML_ELEMENT_NODE == node->type) {
            const char *value = XmlUtils::value(node);
            if (value && strncasecmp(reinterpret_cast<const char*>(node->name), "method", sizeof("method")) == 0) {
                method = value;
                break;
            }
        }
        node = node->next;
    }
    if (method.empty()) {
        throw std::runtime_error("method is not provided");
    }

    return findConstructor(method)(this, owner, orig);
}

void
ControlExtension::registerConstructor(const std::string & method, Constructor ctor) {
    constructors()[method] = ctor;
}

ControlExtension::Constructor
ControlExtension::findConstructor(const std::string& method) {
    ConstructorMap::const_iterator m = constructors().find(method);
    if (m == constructors().end()) {
        throw std::runtime_error("method doesn't exists");
    }
    return m->second;
}

void
ControlExtension::setControlFlag(Context *ctx) {
    try {
        ctx->param(CONTROL_KEY, true);
    }
    catch(const std::invalid_argument&) {
    }
}

bool
ControlExtension::isControl(const Context *ctx) {
    try {
        return ctx->param<bool>(CONTROL_KEY);
    }
    catch(const std::invalid_argument&) {
    }
    
    return false;
}

// We should not register ControlExtension. It will be registered in Config::startup
//REGISTER_COMPONENT(ControlExtension);

} // namespace xscript


