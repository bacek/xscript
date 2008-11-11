#include "settings.h"

#include "xscript/validator.h"
#include "xscript/validator_exception.h"
#include "xscript/validator_factory.h"
#include "xscript/xml_util.h"
#include "xscript/context.h"
#include "xscript/param.h"
#include "xscript/state.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

Validator::Validator(xmlNodePtr node) {
    xmlAttrPtr attr = xmlHasProp(node, (const xmlChar*) "validate-error-guard");
    if (attr) {
        guard_name_ = XmlUtils::value(attr);
        xmlRemoveProp(attr); // libxml will free memory
    }
}

Validator::~Validator() {
}

void 
Validator::check(const Context *ctx, const Param &param) const {
    try {
        checkImpl(ctx, param);
    }
    catch(ValidatorException &ex) {
        const std::string &id = param.id();
        if (!id.empty())
            ex.add("param-id", id);

        if (!guard_name_.empty()) {
            ctx->state()->setBool(guard_name_, true);
        }
        throw;
    }

}

ValidatorRegisterer::ValidatorRegisterer(const char *name, const ValidatorConstructor &cons) {
    ValidatorFactory::instance()->registerConstructor(name, cons);
}

} // namespace xscript

