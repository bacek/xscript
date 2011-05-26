#include "settings.h"

#include <sstream>

#include "xscript/exception.h"
#include "xscript/context.h"
#include "xscript/logger.h"
#include "xscript/message_interface.h"
#include "xscript/operation_mode.h"
#include "xscript/remote_tagged_block.h"
#include "xscript/request.h"
#include "xscript/response.h"
#include "xscript/script.h"
#include "xscript/stylesheet.h"
#include "xscript/util.h"
#include "xscript/vhost_data.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class DevelopmentMode : public OperationMode {
public:
    DevelopmentMode();
    virtual ~DevelopmentMode();
    virtual void processError(const std::string& message);
    virtual void processCriticalInvokeError(const std::string& message);
    virtual void sendError(Response* response, unsigned short status, const std::string& message);
    virtual bool isProduction();
    virtual void assignBlockError(Context *ctx, const Block *block, const std::string &error);
    virtual void processPerblockXsltError(const Context *ctx, const InvokeContext *invoke_ctx, const Block *block);
    virtual void processScriptError(const Context *ctx, const Script *script);
    virtual void processMainXsltError(const Context *ctx, const Script *script, const Stylesheet *style);
    virtual void processXmlError(const std::string &filename);
    virtual void collectError(const InvokeError &error, InvokeError &full_error);
    virtual bool checkDevelopmentVariable(const Request* request, const std::string &var);
    virtual void checkRemoteTimeout(RemoteTaggedBlock *block);
};

DevelopmentMode::DevelopmentMode()
{}

DevelopmentMode::~DevelopmentMode()
{}

void
DevelopmentMode::processError(const std::string &message) {
    throw UnboundRuntimeError(message);
}

void
DevelopmentMode::processCriticalInvokeError(const std::string &message) {
    throw CriticalInvokeError(message);
}

void
DevelopmentMode::sendError(Response* response, unsigned short status, const std::string &message) {
    response->sendError(status, message);
}

bool
DevelopmentMode::isProduction() {
    return false;
}

void
DevelopmentMode::assignBlockError(Context *ctx, const Block *block, const std::string &error) {
    ctx->assignRuntimeError(block, error);
}

void
DevelopmentMode::processPerblockXsltError(const Context *ctx,
    const InvokeContext *invoke_ctx, const Block *block) {
    std::string res = ctx->getRuntimeError(block);
    if (!res.empty()) {
        throw CriticalInvokeError(res, "xslt", invoke_ctx->xsltName());
    }
    if (XmlUtils::hasXMLError()) {
        std::string error = XmlUtils::getXMLError();
        if (!error.empty()) {
            throw InvokeError(error, "xslt", invoke_ctx->xsltName());
        }
    }
}

void
DevelopmentMode::processScriptError(const Context *ctx, const Script *script) {
    std::string res;
    unsigned int size = script->blocksNumber();
    for (unsigned int i = 0; i < size; ++i) {
        std::string error = ctx->getRuntimeError(script->block(i));
        if (!error.empty()) {
            res.append(error);
            res.push_back(' ');
        }
    }
    if (!res.empty()) {
        throw InvokeError(res.c_str());
    }
}

void
DevelopmentMode::processMainXsltError(const Context *ctx, const Script *script, const Stylesheet *style) {
    std::string res = ctx->getRuntimeError(NULL);
    if (!res.empty()) {
        std::stringstream stream;
        stream << res << ". Script: " << script->name() << ". Main stylesheet: " << style->name();
        throw InvokeError(stream.str());
    }
    if (XmlUtils::hasXMLError()) {
        std::string error = XmlUtils::getXMLError();
        if (!error.empty()) {
            std::stringstream stream;
            stream << error << ". Script: " << script->name() << ". Main stylesheet: " << style->name();
            throw InvokeError(stream.str());
        }
    }
}

void
DevelopmentMode::processXmlError(const std::string &filename) {
    if (XmlUtils::hasXMLError()) {
        std::string error = XmlUtils::getXMLError();
        if (!error.empty()) {
            std::stringstream stream;
            stream << error << ". File: " << filename;
            throw UnboundRuntimeError(stream.str());
        }
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
    if (block->retryCount() == 0 && !block->tagged() && !block->isDefaultRemoteTimeout()) {
        throw std::runtime_error("remote timeout setup is prohibited for non-tagged blocks or when tag cache time is nil");
    }
}

static ComponentImplRegisterer<OperationMode> reg_(new DevelopmentMode());

} // namespace xscript
