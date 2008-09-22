#include "xscript/validator_factory.h"

namespace xscript
{

REGISTER_COMPONENT(ValidatorFactory);

ValidatorFactory::ValidatorFactory() {
}

ValidatorFactory::~ValidatorFactory() {
}

std::auto_ptr<ValidatorBase>
ValidatorFactory::createValidator(xmlNodePtr node) {
    return std::auto_ptr<ValidatorBase>();
}

}
