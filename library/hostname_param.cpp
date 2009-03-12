#include "settings.h"

#include "xscript/args.h"
#include "xscript/context.h"
#include "xscript/param.h"
#include "xscript/request.h"
#include "xscript/vhost_data.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class HostnameParam : public Param {
public:
    HostnameParam(Object *owner, xmlNodePtr node);
    virtual ~HostnameParam();

    virtual void parse();

    virtual const char* type() const;
    virtual std::string asString(const Context *ctx) const;
    virtual void add(const Context *ctx, ArgList &al) const;

    static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);

};

HostnameParam::HostnameParam(Object *owner, xmlNodePtr node) :
        Param(owner, node) {
}

HostnameParam::~HostnameParam() {
}

const char*
HostnameParam::type() const {
    return "Hostname";
}

void
HostnameParam::parse() {

    Param::parse();

    if ( !value().empty() ) {
         throw std::runtime_error("hostname param must have an empty value");
    }
}

std::string
HostnameParam::asString(const Context *ctx) const {
    (void)ctx;
    return VirtualHostData::instance()->getServer()->hostname();
}

void
HostnameParam::add(const Context *ctx, ArgList &al) const {
    al.add(asString(ctx));
}

std::auto_ptr<Param>
HostnameParam::create(Object *owner, xmlNodePtr node) {
    return std::auto_ptr<Param>(new HostnameParam(owner, node));
}

static CreatorRegisterer reg_("hostname", &HostnameParam::create);

} // namespace xscript
