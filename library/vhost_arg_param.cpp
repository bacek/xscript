#include "settings.h"

#include "xscript/context.h"
#include "xscript/param.h"
#include "xscript/vhost_data.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class VHostArgParam : public ConvertedParam {
public:
    VHostArgParam(Object *owner, xmlNodePtr node);
    virtual ~VHostArgParam();

    virtual const char* type() const;
    virtual std::string asString(const Context *ctx) const;

    static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
};

VHostArgParam::VHostArgParam(Object *owner, xmlNodePtr node) :
    ConvertedParam(owner, node) {
}

VHostArgParam::~VHostArgParam() {
}

const char*
VHostArgParam::type() const {
    return "VHostArg";
}

std::string
VHostArgParam::asString(const Context *ctx) const {
    (void)ctx;
    const std::string &param_name = value();
    if (strncasecmp(param_name.c_str(), "noxslt-port", sizeof("noxslt-port") - 1) == 0) {
        return boost::lexical_cast<std::string>(
                VirtualHostData::instance()->getServer()->noXsltPort());
    }
    else if (strncasecmp(param_name.c_str(), "alternate-port", sizeof("alternate-port") - 1) == 0) {
        return boost::lexical_cast<std::string>(
                VirtualHostData::instance()->getServer()->alternatePort());
    }

    throw std::runtime_error(std::string("Unknown virtual host arg: ") + param_name);
}

std::auto_ptr<Param>
VHostArgParam::create(Object *owner, xmlNodePtr node) {
    return std::auto_ptr<Param>(new VHostArgParam(owner, node));
}

static CreatorRegisterer reg_("vhostarg", &VHostArgParam::create);

} // namespace xscript
