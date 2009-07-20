#include "settings.h"

#include <stdexcept>

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
#include "xscript/string_utils.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

const std::string OperationMode::PROCESS_ERROR_METHOD = "OPERATION_MODE_PROCESS_ERROR";
const std::string OperationMode::SEND_ERROR_METHOD = "OPERATION_MODE_SEND_ERROR";
const std::string OperationMode::IS_PRODUCTION_METHOD = "OPERATION_MODE_IS_PRODUCTION";
const std::string OperationMode::ASSIGN_BLOCK_ERROR_METHOD = "OPERATION_MODE_ASSIGN_BLOCK_ERROR";
const std::string OperationMode::PROCESS_PERBLOCK_XSLT_ERROR_METHOD = "OPERATION_MODE_PROCESS_PERBLOCK_XSLT_ERROR";
const std::string OperationMode::PROCESS_SCRIPT_ERROR_METHOD = "OPERATION_MODE_PROCESS_SCRIPT_ERROR";
const std::string OperationMode::PROCESS_MAIN_XSLT_ERROR_METHOD = "OPERATION_MODE_PROCESS_MAIN_XSLT_ERROR";
const std::string OperationMode::COLLECT_ERROR_METHOD = "OPERATION_MODE_COLLECT_ERROR";
const std::string OperationMode::CHECK_DEVELOPMENT_VARIABLE_METHOD = "OPERATION_MODE_CHECK_DEVELOPMENT_VARIABLE";
const std::string OperationMode::CHECK_REMOTE_TIMEOUT_METHOD = "OPERATION_MODE_CHECK_REMOTE_TIMEOUT";

void
OperationMode::processError(const std::string &message) {
    MessageParam<const std::string> message_param(&message);
    
    MessageParamBase* param_list[1];
    param_list[0] = &message_param;
    
    MessageParams params(1, param_list);
    MessageResultBase result;
  
    MessageProcessor::instance()->process(PROCESS_ERROR_METHOD, params, result);
}

void
OperationMode::sendError(Response* response, unsigned short status, const std::string &message) {
    MessageParam<Response> response_param(response);
    MessageParam<unsigned short> status_param(&status);
    MessageParam<const std::string> message_param(&message);
    
    MessageParamBase* param_list[3];
    param_list[0] = &response_param;
    param_list[1] = &status_param;
    param_list[2] = &message_param;
    
    MessageParams params(3, param_list);
    MessageResultBase result;
  
    MessageProcessor::instance()->process(SEND_ERROR_METHOD, params, result);
}

bool
OperationMode::isProduction() {
    MessageParams params;
    MessageResult<bool> result;
    MessageProcessor::instance()->process(IS_PRODUCTION_METHOD, params, result);
    return result.get();
}

void
OperationMode::assignBlockError(Context *ctx, const Block *block, const std::string &error) {
    MessageParam<Context> context_param(ctx);
    MessageParam<const Block> block_param(block);
    MessageParam<const std::string> error_param(&error);
    
    MessageParamBase* param_list[3];
    param_list[0] = &context_param;
    param_list[1] = &block_param;
    param_list[2] = &error_param;
    
    MessageParams params(3, param_list);
    MessageResultBase result;
  
    MessageProcessor::instance()->process(ASSIGN_BLOCK_ERROR_METHOD, params, result);
}

void
OperationMode::processPerblockXsltError(const Context *ctx, const Block *block) {
    MessageParam<const Context> context_param(ctx);
    MessageParam<const Block> block_param(block);
    
    MessageParamBase* param_list[2];
    param_list[0] = &context_param;
    param_list[1] = &block_param;
    
    MessageParams params(2, param_list);
    MessageResultBase result;
  
    MessageProcessor::instance()->process(PROCESS_PERBLOCK_XSLT_ERROR_METHOD, params, result);
}

void
OperationMode::processScriptError(const Context *ctx, const Script *script) {
    MessageParam<const Context> context_param(ctx);
    MessageParam<const Script> script_param(script);
    
    MessageParamBase* param_list[2];
    param_list[0] = &context_param;
    param_list[1] = &script_param;
    
    MessageParams params(2, param_list);
    MessageResultBase result;
  
    MessageProcessor::instance()->process(PROCESS_SCRIPT_ERROR_METHOD, params, result);
}

void
OperationMode::processMainXsltError(const Context *ctx, const Script *script, const Stylesheet *style) {
    MessageParam<const Context> context_param(ctx);
    MessageParam<const Script> script_param(script);
    MessageParam<const Stylesheet> stylesheet_param(style);
    
    MessageParamBase* param_list[3];
    param_list[0] = &context_param;
    param_list[1] = &script_param;
    param_list[2] = &stylesheet_param;
    
    MessageParams params(3, param_list);
    MessageResultBase result;
  
    MessageProcessor::instance()->process(PROCESS_SCRIPT_ERROR_METHOD, params, result);
}

void
OperationMode::collectError(const InvokeError &error, InvokeError &full_error) {  
    MessageParam<const InvokeError> error_param(&error);
    MessageParam<InvokeError> full_error_param(&full_error);
    
    MessageParamBase* param_list[2];
    param_list[0] = &error_param;
    param_list[1] = &full_error_param;
    
    MessageParams params(2, param_list);
    MessageResultBase result;
  
    MessageProcessor::instance()->process(COLLECT_ERROR_METHOD, params, result);
}

bool
OperationMode::checkDevelopmentVariable(const Request* request, const std::string &var) {   
    MessageParam<const Request> request_param(request);
    MessageParam<const std::string> var_param(&var);
    
    MessageParamBase* param_list[2];
    param_list[0] = &request_param;
    param_list[1] = &var_param;
    
    MessageParams params(2, param_list);
    MessageResult<bool> result;
    
    MessageProcessor::instance()->process(CHECK_DEVELOPMENT_VARIABLE_METHOD, params, result);
    return result.get();
}

void
OperationMode::checkRemoteTimeout(RemoteTaggedBlock *block) {
    MessageParam<RemoteTaggedBlock> block_param(block);
    
    MessageParamBase* param_list[1];
    param_list[0] = &block_param;
    
    MessageParams params(1, param_list);
    MessageResultBase result;
  
    MessageProcessor::instance()->process(CHECK_REMOTE_TIMEOUT_METHOD, params, result);
}

namespace OperationModeHandlers {

class ProcessErrorHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        (void)result;
        const std::string* message = params.getParam<const std::string>(0);
        log()->warn("%s", message->c_str());
        return -1;
    }
};

class SendErrorHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        (void)result;
        Response* response = params.getParam<Response>(0);
        unsigned short* status = params.getParam<unsigned short>(1);
        response->sendError(*status, StringUtils::EMPTY_STRING);
        return -1;
    }
};

class IsProductionHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        (void)params;
        result.set(true);
        return -1;
    }
};

class ProcessPerblockXsltErrorHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        (void)result;
        const Block* block = params.getParam<const Block>(1);
        if (XmlUtils::hasXMLError()) {
            std::string postfix = "Script: " + block->owner()->name() +
                ". Block: name: " + block->name() + ", id: " + block->id() +
                ", method: " + block->method() + ". Perblock stylesheet: " + block->xsltName();
            XmlUtils::printXMLError(postfix);
        }
        return -1;
    }
};

class ProcessMainXsltErrorHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        (void)result;
        const Script* script = params.getParam<const Script>(1);
        const Stylesheet* style = params.getParam<const Stylesheet>(2);
        if (XmlUtils::hasXMLError()) {
            std::string postfix = "Script: " + script->name() + ". Main stylesheet: " + style->name();
            XmlUtils::printXMLError(postfix);
        }
        return -1;
    }
};

class CollectErrorHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        (void)result;
        const InvokeError* error = params.getParam<const InvokeError>(0);
        InvokeError* full_error = params.getParam<InvokeError>(1);
        
        std::stringstream stream;
        stream << error->what() << ": " << full_error->what_info() << "";
        
        const InvokeError::InfoMapType& info = error->info();
        for(InvokeError::InfoMapType::const_iterator it = info.begin();
            it != info.end();
            ++it) {
            stream << ". " << it->first << ": " << it->second;        
        }
        
        log()->error("%s", stream.str().c_str());  
        return -1;
    }
};

class CheckDevelopmentVariableHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        (void)params;
        result.set(false);
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
            
            block->setDefaultRemoteTimeout();
            log()->warn("remote timeout setup is prohibited for non-tagged blocks or when tag cache time is nil: %s",
                    block->owner()->name().c_str());
        }
        return -1;
    }
};

struct HandlerRegisterer {
    HandlerRegisterer() {
        MessageProcessor::instance()->registerBack(OperationMode::PROCESS_ERROR_METHOD,
                boost::shared_ptr<MessageHandler>(new ProcessErrorHandler()));
        MessageProcessor::instance()->registerBack(OperationMode::SEND_ERROR_METHOD,
                boost::shared_ptr<MessageHandler>(new SendErrorHandler()));
        MessageProcessor::instance()->registerBack(OperationMode::IS_PRODUCTION_METHOD,
                boost::shared_ptr<MessageHandler>(new IsProductionHandler()));
        MessageProcessor::instance()->registerBack(OperationMode::ASSIGN_BLOCK_ERROR_METHOD,
                boost::shared_ptr<MessageHandler>(new MessageHandler()));
        MessageProcessor::instance()->registerBack(OperationMode::PROCESS_PERBLOCK_XSLT_ERROR_METHOD,
                boost::shared_ptr<MessageHandler>(new ProcessPerblockXsltErrorHandler()));
        MessageProcessor::instance()->registerBack(OperationMode::PROCESS_SCRIPT_ERROR_METHOD,
                boost::shared_ptr<MessageHandler>(new MessageHandler()));
        MessageProcessor::instance()->registerBack(OperationMode::PROCESS_MAIN_XSLT_ERROR_METHOD,
                boost::shared_ptr<MessageHandler>(new ProcessMainXsltErrorHandler()));
        MessageProcessor::instance()->registerBack(OperationMode::COLLECT_ERROR_METHOD,
                boost::shared_ptr<MessageHandler>(new CollectErrorHandler()));
        MessageProcessor::instance()->registerBack(OperationMode::CHECK_DEVELOPMENT_VARIABLE_METHOD,
                boost::shared_ptr<MessageHandler>(new CheckDevelopmentVariableHandler()));
        MessageProcessor::instance()->registerBack(OperationMode::CHECK_REMOTE_TIMEOUT_METHOD,
                boost::shared_ptr<MessageHandler>(new CheckRemoteTimeoutHandler()));
    }
};

static HandlerRegisterer reg_handlers;

} // namespace OperationModeHandlers
} // namespace xscript
