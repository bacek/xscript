#include "settings.h"
#include "xscript/logger.h"
#include "xscript/operation_mode.h"
#include "xscript/response.h"
#include "xscript/util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class DevelopmentMode : public OperationMode {
public:
    DevelopmentMode();
    virtual ~DevelopmentMode();
    virtual void processError(const std::string& message);
    virtual void sendError(Response* response, unsigned short status, const std::string& message);
    virtual bool isProduction() const;
};

DevelopmentMode::DevelopmentMode() {
}

DevelopmentMode::~DevelopmentMode() {
}

void
DevelopmentMode::processError(const std::string& message) {
    log()->error("%s", message.c_str());
    throw UnboundRuntimeError(message);
}

void
DevelopmentMode::sendError(Response* response, unsigned short status, const std::string& message) {
    response->sendError(status, message);
}

bool
DevelopmentMode::isProduction() const {
    return false;
}

static ComponentRegisterer<OperationMode> reg_(new DevelopmentMode());

} // namespace xscript
