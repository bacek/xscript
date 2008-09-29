#include "xscript/validator_base.h"
#include "xscript/validator_exception.h"
#include "xscript/xml_util.h"
#include "xscript/context.h"
#include "xscript/state.h"

namespace xscript
{

ValidatorBase::ValidatorBase(xmlNodePtr node) {
    xmlAttrPtr attr = xmlHasProp(node, (const xmlChar*) "validate-error-guard");
    if (attr) {
        guard_name_ = XmlUtils::value(attr);
        xmlRemoveProp(attr); // libxml will free memory
    }
}

ValidatorBase::~ValidatorBase() {
}

void 
ValidatorBase::check(const Context *ctx, const std::string &value) const {
    if (isFailed(value)) {
        if (!guard_name_.empty()) {
            ctx->state()->setBool(guard_name_, true);
        }
        throw ValidatorException();
    }
}

}
