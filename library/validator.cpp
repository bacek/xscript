#include "xscript/validator.h"
#include "xscript/validator_exception.h"
#include "xscript/validator_factory.h"
#include "xscript/xml_util.h"
#include "xscript/context.h"
#include "xscript/state.h"

namespace xscript
{

Validator::Validator(xmlNodePtr node) {
    xmlAttrPtr attr = xmlHasProp(node, (const xmlChar*) "validate-error-guard");
    if (attr) {
        guard_name_ = XmlUtils::value(attr);
        xmlRemoveProp(attr); // libxml will free memory
    }
    attr = xmlHasProp(node, (const xmlChar*) "id");
    if (attr)
        param_id_ = XmlUtils::value(attr);
}

Validator::~Validator() {
}

void 
Validator::check(const Context *ctx, const Param &param) const {
    try {
        checkImpl(ctx, param);
    }
    catch(ValidatorException &ex) {
        if (!param_id_.empty())
            ex.add("param-id", param_id_);

        if (!guard_name_.empty()) {
            ctx->state()->setBool(guard_name_, true);
        }
        throw;
    }

}

ValidatorRegisterer::ValidatorRegisterer(const char *name, const ValidatorConstructor &cons) {
    ValidatorFactory::instance()->registerConstructor(name, cons);
}

}
