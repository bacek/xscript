#include "settings.h"

#include "xscript/context.h"
#include "xscript/guard_checker.h"
#include "xscript/vhost_data.h"

#include "internal/vhost_arg_param.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

VHostArgParam::VHostArgParam(Object *owner, xmlNodePtr node) :
    TypedParam(owner, node) {
}

VHostArgParam::~VHostArgParam() {
}

const char*
VHostArgParam::type() const {
    return "VHostArg";
}

std::string
VHostArgParam::variable(const Context *ctx, const std::string &name) {
    if (strncmp(name.c_str(), "XSCRIPT_", sizeof("XSCRIPT_") - 1) == 0) {
        return VirtualHostData::instance()->getVariable(ctx->rootContext()->request(), name);
    }
    std::string value;
    const Config *config = VirtualHostData::instance()->getConfig();
    if (config->getCacheParam(name, value)) {
        return value;
    }
    return StringUtils::EMPTY_STRING;
}

TypedParam::ValueResult
VHostArgParam::getValue(const Context *ctx) const {
    std::string result = variable(ctx, value());
    return ValueResult(result, !result.empty());
}

std::auto_ptr<Param>
VHostArgParam::create(Object *owner, xmlNodePtr node) {
    return std::auto_ptr<Param>(new VHostArgParam(owner, node));
}

bool
VHostArgParam::is(const Context *ctx, const std::string &name, const std::string &value) {
    if (value.empty()) {
        return !variable(ctx, name).empty();
    }
    return variable(ctx, name) == value;
}

static CreatorRegisterer reg_("vhostarg", &VHostArgParam::create);
static GuardCheckerRegisterer reg2_("vhostarg", &VHostArgParam::is, false);

} // namespace xscript
