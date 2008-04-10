#include "xscript/block.h"
#include "xscript/util.h"
#include "xscript/control_extension.h"

namespace xscript
{

ControlExtension::ControlExtension() {
}

ControlExtension::~ControlExtension() {
}
	
const char* ControlExtension::name() const {
    return "control";
}

const char* ControlExtension::nsref() const {
    return "http://www.yandex.ru/xscript";
}
	
void ControlExtension::initContext(Context *) {
}

void ControlExtension::stopContext(Context *) {
}

void ControlExtension::destroyContext(Context *) {
}
	
std::auto_ptr<Block> ControlExtension::createBlock(Xml *owner, xmlNodePtr orig) {
    std::string method;
	xmlNodePtr node = orig->children;
	while (node) {
		if (!node->name) {
			continue;
		}
		else if (XML_ELEMENT_NODE == node->type) {
            const char *value = XmlUtils::value(node);
			if(value && strncasecmp(reinterpret_cast<const char*>(node->name), "method", sizeof("method")) == 0) {
				method = value;
                break;
			}
		}
		node = node->next;
	}
    if(method.empty()) {
        throw std::runtime_error("method is not provided");
    }

    return ControlExtensionRegistry::findConstructor(method)(this, owner, orig);
}


ControlExtensionRegistry::constructorMap_t ControlExtensionRegistry::constructors_;

void ControlExtensionRegistry::registerConstructor(const std::string & method, constructor_t ctor) {
    constructors_[method] = ctor;
}

ControlExtensionRegistry::constructor_t
ControlExtensionRegistry::findConstructor(const std::string& method) {
    constructorMap_t::const_iterator m = constructors_.find(method);
    if(m == constructors_.end()) {
        throw std::runtime_error("method doesn't exists");
    }
    return m->second;
}

// We should not register ControlExtension. It will be registred in Config::startup
//REGISTER_COMPONENT(ControlExtension);

}


