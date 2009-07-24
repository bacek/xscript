#include "settings.h"

#include <sstream>

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
namespace DevelopmentModeHandlers {

class ProcessErrorHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        (void)result;
        const std::string* message = params.getParam<const std::string>(0);
        log()->error("%s", message->c_str());
        throw UnboundRuntimeError(*message);
    }
};

class SendErrorHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        (void)result;
        Response* response = params.getParam<Response>(0);
        unsigned short* status = params.getParam<unsigned short>(1);
        const std::string *message = params.getParam<const std::string>(2);
        response->sendError(*status, *message);
        return -1;
    }
};

class IsProductionHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        (void)params;
        result.set(false);
        return -1;
    }
};

class AssignBlockErrorHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        (void)result;
        Context* ctx = params.getParam<Context>(0);
        const Block* block = params.getParam<const Block>(1);
        const std::string *error = params.getParam<const std::string>(2);
        ctx->assignRuntimeError(block, *error);
        return -1;
    }
};

class ProcessPerblockXsltErrorHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        (void)result;
        const Context* ctx = params.getParam<Context>(0);
        const Block* block = params.getParam<const Block>(1);
        std::string res = ctx->getRuntimeError(block);
        if (!res.empty()) {
            throw CriticalInvokeError(res, "xslt", block->xsltName());
        }
        std::string error = XmlUtils::getXMLError();
        if (!error.empty()) {
            throw InvokeError(error, "xslt", block->xsltName());
        }
        return -1;
    }
};

class ProcessScriptErrorHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        (void)result;
        
        const Context* ctx = params.getParam<const Context>(0);
        const Script* script = params.getParam<const Script>(1);
        
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
        
        return -1;
    }
};

class ProcessMainXsltErrorHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        (void)result;
        
        const Context* ctx = params.getParam<const Context>(0);
        const Script* script = params.getParam<const Script>(1);
        const Stylesheet* style = params.getParam<const Stylesheet>(2);
        
        std::string res = ctx->getRuntimeError(NULL);
        if (!res.empty()) {            
            std::stringstream stream;
            stream << res << ". Script: " << script->name() << ". Main stylesheet: " << style->name();
            throw InvokeError(stream.str());
        }
        std::string error = XmlUtils::getXMLError();
        if (!error.empty()) {
            std::stringstream stream;
            stream << error << ". Script: " << script->name() << ". Main stylesheet: " << style->name();
            throw InvokeError(stream.str());
        }
        
        return -1;
    }
};

class CollectErrorHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        (void)result;
        const InvokeError* error = params.getParam<const InvokeError>(0);
        InvokeError* full_error = params.getParam<InvokeError>(1);
        
        const InvokeError::InfoMapType& info = error->info();
        for(InvokeError::InfoMapType::const_iterator it = info.begin();
            it != info.end();
            ++it) {
            full_error->add(it->first, it->second);        
        } 
 
        return -1;
    }
};

class CheckDevelopmentVariableHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        const Request* request = params.getParam<const Request>(0);
        const std::string* var = params.getParam<const std::string>(1);
        result.set(VirtualHostData::instance()->checkVariable(request, *var));
        return -1;
    }
};

class CheckRemoteTimeoutHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        (void)result;
        RemoteTaggedBlock* block = params.getParam<RemoteTaggedBlock>(0);
        if (block->retryCount() == 0 &&
            !block->tagged() &&
            !block->isDefaultRemoteTimeout()) {
            
            throw std::runtime_error("remote timeout setup is prohibited for non-tagged blocks or when tag cache time is nil");
        }
        return -1;
    }
};

struct HandlerRegisterer {
    HandlerRegisterer() {
        MessageProcessor::instance()->registerFront(OperationMode::PROCESS_ERROR_METHOD,
                boost::shared_ptr<MessageHandler>(new ProcessErrorHandler()));
        MessageProcessor::instance()->registerFront(OperationMode::SEND_ERROR_METHOD,
                boost::shared_ptr<MessageHandler>(new SendErrorHandler()));
        MessageProcessor::instance()->registerFront(OperationMode::IS_PRODUCTION_METHOD,
                boost::shared_ptr<MessageHandler>(new IsProductionHandler()));
        MessageProcessor::instance()->registerFront(OperationMode::ASSIGN_BLOCK_ERROR_METHOD,
                boost::shared_ptr<MessageHandler>(new AssignBlockErrorHandler()));
        MessageProcessor::instance()->registerFront(OperationMode::PROCESS_PERBLOCK_XSLT_ERROR_METHOD,
                boost::shared_ptr<MessageHandler>(new ProcessPerblockXsltErrorHandler()));
        MessageProcessor::instance()->registerFront(OperationMode::PROCESS_SCRIPT_ERROR_METHOD,
                boost::shared_ptr<MessageHandler>(new ProcessScriptErrorHandler()));
        MessageProcessor::instance()->registerFront(OperationMode::PROCESS_MAIN_XSLT_ERROR_METHOD,
                boost::shared_ptr<MessageHandler>(new ProcessMainXsltErrorHandler()));
        MessageProcessor::instance()->registerFront(OperationMode::COLLECT_ERROR_METHOD,
                boost::shared_ptr<MessageHandler>(new CollectErrorHandler()));
        MessageProcessor::instance()->registerFront(OperationMode::CHECK_DEVELOPMENT_VARIABLE_METHOD,
                boost::shared_ptr<MessageHandler>(new CheckDevelopmentVariableHandler()));
        MessageProcessor::instance()->registerFront(OperationMode::CHECK_REMOTE_TIMEOUT_METHOD,
                boost::shared_ptr<MessageHandler>(new CheckRemoteTimeoutHandler()));
    }
};

static HandlerRegisterer reg_handlers;

} // namespace DevelopmentModeHandlers
} // namespace xscript
