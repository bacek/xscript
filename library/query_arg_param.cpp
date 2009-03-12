#include "settings.h"

#include "xscript/param.h"
#include "xscript/request.h"
#include "xscript/context.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class QueryArgParam : public TypedParam {
public:
    QueryArgParam(Object *owner, xmlNodePtr node);
    virtual ~QueryArgParam();

    virtual const char* type() const;
    virtual std::string asString(const Context *ctx) const;

    static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
};

QueryArgParam::QueryArgParam(Object *owner, xmlNodePtr node) :
        TypedParam(owner, node) {
}

QueryArgParam::~QueryArgParam() {
}

const char*
QueryArgParam::type() const {
    return "QueryArg";
}

std::string
QueryArgParam::asString(const Context *ctx) const {
    Request *req = ctx->request();
    if (req->hasArg(value())) {
        return req->getArg(value());
    }
    return defaultValue();
}

std::auto_ptr<Param>
QueryArgParam::create(Object *owner, xmlNodePtr node) {
    return std::auto_ptr<Param>(new QueryArgParam(owner, node));
}

static CreatorRegisterer reg_("queryarg", &QueryArgParam::create);

} // namespace xscript
