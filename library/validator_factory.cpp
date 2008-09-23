#include "xscript/validator_factory.h"
#include "xscript/xml_util.h"

namespace xscript
{

REGISTER_COMPONENT(ValidatorFactory);

ValidatorFactory::ValidatorFactory() {
}

ValidatorFactory::~ValidatorFactory() {
}

std::auto_ptr<ValidatorBase>
ValidatorFactory::createValidator(xmlNodePtr node) {
    std::string type;
    xmlAttrPtr attr = node->properties;
    while (attr) {
        const char *name = (const char*) attr->name;
        if (strcmp("validator", name) == 0) {
            type = XmlUtils::value(attr);
            break;
        }
        attr = attr->next;
    }

    // If validator absent for param return null.
    if (type.empty())
        return std::auto_ptr<ValidatorBase>();

    ValidatorMap::const_iterator i = validator_creators_.find(type);
    if (i == validator_creators_.end())
        throw std::runtime_error("Unknown validator type: " + type);

    return std::auto_ptr<ValidatorBase>((*i->second)(node));
}

}
