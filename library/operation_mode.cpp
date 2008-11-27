#include "settings.h"
#include "xscript/logger.h"
#include "xscript/operation_mode.h"
#include "xscript/response.h"
#include "xscript/util.h"

#include <stdexcept>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

OperationMode::OperationMode() {
}

OperationMode::~OperationMode() {
}

void
OperationMode::processError(const std::string& message) {
    log()->warn("%s", message.c_str());
}

void
OperationMode::sendError(Response* response, unsigned short status, const std::string& message) {
    (void)message;
    response->sendError(status, StringUtils::EMPTY_STRING);
}

bool
OperationMode::isProduction() const {
    return true;
}

REGISTER_COMPONENT(OperationMode);

} // namespace xscript
