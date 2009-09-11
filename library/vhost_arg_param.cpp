#include "settings.h"

#include "xscript/context.h"
#include "xscript/guard_checker.h"
#include "xscript/param.h"
#include "xscript/vhost_data.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class VHostArgParam : public TypedParam {
public:
    VHostArgParam(Object *owner, xmlNodePtr node);
    virtual ~VHostArgParam();

    virtual const char* type() const;
    virtual std::string asString(const Context *ctx) const;

    static std::string variable(const Context *ctx, const std::string &name);
    
    static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
    static bool is(const Context *ctx, const std::string &name, const std::string &value);
};

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
    VirtualHostData *vhdata = VirtualHostData::instance();
    if (strncasecmp(name.c_str(), "noxslt-port", sizeof("noxslt-port")) == 0) {
        return boost::lexical_cast<std::string>(vhdata->getServer()->noXsltPort());
    }
    else if (strncasecmp(name.c_str(), "alternate-port", sizeof("alternate-port")) == 0) {
        return boost::lexical_cast<std::string>(vhdata->getServer()->alternatePort());
    }
    
    if (strncmp(name.c_str(), "XSCRIPT_", sizeof("XSCRIPT_") - 1) != 0) {
        throw std::runtime_error(
            "Environment variable without XSCRIPT prefix is not allowed in VHostArg: " + name);
    }
    return vhdata->getVariable(ctx->request(), name);
}

std::string
VHostArgParam::asString(const Context *ctx) const {
    std::string result = variable(ctx, value());
    return result.empty() ? defaultValue() : result;
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
