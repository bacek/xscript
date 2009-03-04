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
#include "xscript/vhost_data.h"

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
    virtual void assignBlockError(Context *ctx, const Block *block, const std::string &error);
    virtual void processPerblockXsltError(const Context *ctx, const Block *block);
    virtual void processScriptError(const Context *ctx, const Script *script);
    virtual void processMainXsltError(const Context *ctx, const Script *script, const Stylesheet *style);
    virtual void collectError(const InvokeError &error, InvokeError &full_error);
    virtual bool checkDevelopmentVariable(const Request* request, const std::string &var);
    virtual void checkRemoteTimeout(RemoteTaggedBlock *block);
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

void
DevelopmentMode::assignBlockError(Context *ctx, const Block *block, const std::string &error) {
    ctx->assignRuntimeError(block, error);
}

void
DevelopmentMode::processPerblockXsltError(const Context *ctx, const Block *block) {
    std::string result = ctx->getRuntimeError(block);
    if (!result.empty()) {
        throw CriticalInvokeError(result.c_str(), "xslt", block->xsltName());
    }
}

void
DevelopmentMode::processScriptError(const Context *ctx, const Script *script) {
    std::string result;
    unsigned int size = script->blocksNumber();
    for (unsigned int i = 0; i < size; ++i) {
        std::string error = ctx->getRuntimeError(script->block(i));
        if (!error.empty()) {
            result.append(error);
            result.push_back(' ');
        }
    }
    
    if (!result.empty()) {
        throw InvokeError(result.c_str());
    }
}

void
DevelopmentMode::processMainXsltError(const Context *ctx, const Script *script, const Stylesheet *style) {
    std::string result = ctx->getRuntimeError(NULL);
    if (!result.empty()) {
        std::stringstream stream;
        stream << result << ". Script: " << script->name() << ". Main stylesheet: " << style->name(); 
        throw InvokeError(stream.str());
    }
}

void
DevelopmentMode::collectError(const InvokeError &error, InvokeError &full_error) {
    const InvokeError::InfoMapType& info = error.info();
    for(InvokeError::InfoMapType::const_iterator it = info.begin();
        it != info.end();
        ++it) {
        full_error.add(it->first, it->second);        
    }       
}

bool
DevelopmentMode::checkDevelopmentVariable(const Request* request, const std::string &var) {
    return VirtualHostData::instance()->checkVariable(request, var);
}

void
DevelopmentMode::checkRemoteTimeout(RemoteTaggedBlock *block) {
    if (block->retryCount() == 0 &&
        !block->tagged() &&
        !block->isDefaultRemoteTimeout()) {
        
        throw std::runtime_error("remote timeout setup is prohibited for non-tagged blocks or when tag cache time is nil");
    }
}

static ComponentImplRegisterer<OperationMode> reg_(new DevelopmentMode());

} // namespace xscript
