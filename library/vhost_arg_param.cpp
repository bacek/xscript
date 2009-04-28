#include "settings.h"

#include "xscript/context.h"
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

    static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
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
VHostArgParam::asString(const Context *ctx) const {
    const std::string &param_name = value();
    std::string result;
    if (strncasecmp(param_name.c_str(), "noxslt-port", sizeof("noxslt-port")) == 0) {
        result = boost::lexical_cast<std::string>(
                VirtualHostData::instance()->getServer()->noXsltPort());
    }
    else if (strncasecmp(param_name.c_str(), "alternate-port", sizeof("alternate-port")) == 0) {
        result = boost::lexical_cast<std::string>(
                VirtualHostData::instance()->getServer()->alternatePort());
    }
    else {
        if (strncmp(param_name.c_str(), "XSCRIPT_", sizeof("XSCRIPT_") - 1) != 0) {
            throw std::runtime_error(
                "Environment variable without XSCRIPT prefix is not allowed in VHostArg: " + param_name);
        }
        result = VirtualHostData::instance()->getVariable(ctx->request(), param_name);
    }
    
    return result.empty() ? defaultValue() : result;
}

std::auto_ptr<Param>
VHostArgParam::create(Object *owner, xmlNodePtr node) {
    return std::auto_ptr<Param>(new VHostArgParam(owner, node));
}

static CreatorRegisterer reg_("vhostarg", &VHostArgParam::create);

} // namespace xscript
