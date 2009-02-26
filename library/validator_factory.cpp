#include "settings.h"
#include "xscript/validator_factory.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

REGISTER_COMPONENT(ValidatorFactory);

ValidatorFactory::ValidatorFactory() {
}

ValidatorFactory::~ValidatorFactory() {
}

std::auto_ptr<Validator>
ValidatorFactory::createValidator(xmlNodePtr node) const {
    xmlAttrPtr attr = xmlHasProp(node, (const xmlChar*)"validator");

    // If validator absent for param return null.
    if (!attr)
        return std::auto_ptr<Validator>();

    std::string type = XmlUtils::value(attr);

    // Remove used prop, otherwise Param::parse will throw exception.
    // libxml will free memory
    xmlRemoveProp(attr);

    ValidatorMap::const_iterator i = validator_creators_.find(type);
    if (i == validator_creators_.end())
        throw std::runtime_error("Unknown validator type: " + type);

    return std::auto_ptr<Validator>(i->second(node));
}

void 
ValidatorFactory::registerConstructor(const std::string &type, const ValidatorConstructor &ctor) {
    ValidatorMap::iterator i = validator_creators_.find(type);
    if (i != validator_creators_.end()) 
        throw std::runtime_error("Duplicate validator type: " + type);
    validator_creators_.insert(i, std::make_pair(type, ctor));
}

} // namespace xscript
