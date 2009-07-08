#include "settings.h"
#include "xscript/context.h"
#include "xscript/logger.h"
#include "xscript/operation_mode.h"
#include "xscript/remote_tagged_block.h"
#include "xscript/request.h"
#include "xscript/response.h"
#include "xscript/script.h"
#include "xscript/stylesheet.h"
#include "xscript/util.h"
#include "xscript/string_utils.h"
#include "xscript/xml_util.h"

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

void
OperationMode::assignBlockError(Context *ctx, const Block *block, const std::string &error) {
    (void)ctx;
    (void)block;
    (void)error;
}

void
OperationMode::processPerblockXsltError(const Context *ctx, const Block *block) {
    (void)ctx;
    std::string postfix = "Script: " + block->owner()->name() +
        ". Block: name: " + block->name() + ", id: " + block->id() +
        ", method: " + block->method() + ". Perblock stylesheet: " + block->xsltName();
    XmlUtils::printXMLError(postfix);
}

void
OperationMode::processScriptError(const Context *ctx, const Script *script) {
    (void)ctx;
    (void)script;
}

void
OperationMode::processMainXsltError(const Context *ctx, const Script *script, const Stylesheet *style) {
    if (XmlUtils::hasXMLError()) {
        ctx->rootContext()->setNoCache();
        std::string postfix = "Script: " + script->name() + ". Main stylesheet: " + style->name();
        XmlUtils::printXMLError(postfix);
    }
}

void
OperationMode::collectError(const InvokeError &error, InvokeError &full_error) {  
    std::stringstream stream;
    stream << error.what() << ": " << full_error.what_info() << "";
    
    const InvokeError::InfoMapType& info = error.info();
    for(InvokeError::InfoMapType::const_iterator it = info.begin();
        it != info.end();
        ++it) {
        stream << ". " << it->first << ": " << it->second;        
    }
    
    log()->error("%s", stream.str().c_str());    
}

bool
OperationMode::checkDevelopmentVariable(const Request* request, const std::string &var) {
    (void)request;
    (void)var;
    return false;
}

void
OperationMode::checkRemoteTimeout(RemoteTaggedBlock *block) {
    if (block->retryCount() == 0 &&
        !block->tagged() &&
        !block->isDefaultRemoteTimeout()) {
        
        block->setDefaultRemoteTimeout();
        log()->warn("remote timeout setup is prohibited for non-tagged blocks or when tag cache time is nil: %s",
                block->owner()->name().c_str());
    }
}

static ComponentRegisterer<OperationMode> reg;

} // namespace xscript
