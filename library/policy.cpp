#include "settings.h"
#include <cstring>

#include <boost/current_function.hpp>

#include "xscript/logger.h"
#include "xscript/message_interface.h"
#include "xscript/policy.h"
#include "xscript/request.h"
#include "xscript/util.h"
#include "xscript/string_utils.h"
#include "xscript/vhost_data.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

const std::string Policy::UTF8_ENCODING = "utf-8";

const std::string Policy::REAL_IP_HEADER_NAME_METHOD = "POLICY_REAL_IP_HEADER_NAME";
const std::string Policy::GET_PATH_BY_SCHEME_METHOD = "POLICY_GET_PATH_BY_SCHEME";
const std::string Policy::GET_ROOT_BY_SCHEME_METHOD = "POLICY_GET_ROOT_BY_SCHEME";
const std::string Policy::GET_KEY_METHOD = "POLICY_GET_KEY_METHOD";
const std::string Policy::GET_OUTPUT_ENCODING_METHOD = "POLICY_GET_OUTPUT_ENCODING";
const std::string Policy::USE_DEFAULT_SANITIZER_METHOD = "POLICY_USE_DEFAULT_SANITIZER";
const std::string Policy::PROCESS_CACHE_LEVEL_METHOD = "POLICY_PROCESS_CACHE_LEVEL";
const std::string Policy::ALLOW_CACHING_METHOD = "POLICY_ALLOW_CACHING";
const std::string Policy::ALLOW_CACHING_INPUT_COOKIE_METHOD = "POLICY_ALLOW_CACHING_INPUT_COOKIE";
const std::string Policy::ALLOW_CACHING_OUTPUT_COOKIE_METHOD = "POLICY_ALLOW_CACHING_OUTPUT_COOKIE";
const std::string Policy::IS_SKIPPED_PROXY_HEADER_METHOD = "POLICY_IS_SKIPPED_PROXY_HEADER";
const std::string Policy::IS_ERROR_DOC_METHOD = "POLICY_IS_ERROR_DOC";


class ProxyHeadersHelper {
public:
    static bool skipped(const char* name);

private:
    struct StrSize {
        const char* name;
        int size;
    };

    static const StrSize skipped_headers[];
};

const ProxyHeadersHelper::StrSize
ProxyHeadersHelper::skipped_headers[] = {
    { "host", sizeof("host") },
    { "if-modified-since", sizeof("if-modified-since") },
    { "accept-encoding", sizeof("accept-encoding") },
    { "keep-alive", sizeof("keep-alive") },
    { "connection", sizeof("connection") },
    { "content-length", sizeof("content-length") },
};

bool
ProxyHeadersHelper::skipped(const char* name) {
    for ( int i = sizeof(skipped_headers) / sizeof(skipped_headers[0]); i--; ) {
        const StrSize& sh = skipped_headers[i];
        if (strncasecmp(name, sh.name, sh.size) == 0) {
            return true;
        }
    }

    return false;
}

const std::string&
Policy::realIPHeaderName() {
    MessageParams params;
    MessageResult<const std::string*> result;
    
    MessageProcessor::instance()->process(REAL_IP_HEADER_NAME_METHOD, params, result);
    return *result.get();
}

std::string
Policy::getPathByScheme(const Request *request, const std::string &url) {
    MessageParam<const Request> request_param(request);
    MessageParam<const std::string> url_param(&url);
    
    MessageParamBase* param_list[2];
    param_list[0] = &request_param;
    param_list[1] = &url_param;
    
    MessageParams params(2, param_list);
    MessageResult<std::string> result;
  
    MessageProcessor::instance()->process(GET_PATH_BY_SCHEME_METHOD, params, result);
    return result.get();
}

std::string
Policy::getRootByScheme(const Request *request, const std::string &url) {
    MessageParam<const Request> request_param(request);
    MessageParam<const std::string> url_param(&url);
    
    MessageParamBase* param_list[2];
    param_list[0] = &request_param;
    param_list[1] = &url_param;
    
    MessageParams params(2, param_list);
    MessageResult<std::string> result;
  
    MessageProcessor::instance()->process(GET_ROOT_BY_SCHEME_METHOD, params, result);
    return result.get();
}

std::string
Policy::getKey(const Request* request, const std::string& name) {
    MessageParam<const Request> request_param(request);
    MessageParam<const std::string> name_param(&name);
    
    MessageParamBase* param_list[2];
    param_list[0] = &request_param;
    param_list[1] = &name_param;
    
    MessageParams params(2, param_list);
    MessageResult<std::string> result;
  
    MessageProcessor::instance()->process(GET_KEY_METHOD, params, result);
    return result.get();
}

std::string
Policy::getOutputEncoding(const Request* request) {
    MessageParam<const Request> request_param(request);
    
    MessageParamBase* param_list[1];
    param_list[0] = &request_param;
    
    MessageParams params(1, param_list);
    MessageResult<std::string> result;
  
    MessageProcessor::instance()->process(GET_OUTPUT_ENCODING_METHOD, params, result);
    return result.get();
}

void
Policy::useDefaultSanitizer() {
    MessageParams params;
    MessageResultBase result;
    MessageProcessor::instance()->process(USE_DEFAULT_SANITIZER_METHOD, params, result);
}

bool
Policy::isSkippedProxyHeader(const std::string &header) {
    MessageParam<const std::string> header_param(&header);
    
    MessageParamBase* param_list[1];
    param_list[0] = &header_param;
    
    MessageParams params(1, param_list);
    MessageResult<bool> result;
    
    MessageProcessor::instance()->process(IS_SKIPPED_PROXY_HEADER_METHOD, params, result);
    return result.get();
}

void
Policy::processCacheLevel(TaggedBlock *block, const std::string &no_cache) {
    MessageParam<TaggedBlock> block_param(block);
    MessageParam<const std::string> no_cache_param(&no_cache);
    
    MessageParamBase* param_list[2];
    param_list[0] = &block_param;
    param_list[1] = &no_cache_param;
    
    MessageParams params(2, param_list);
    MessageResultBase result;
    
    MessageProcessor::instance()->process(PROCESS_CACHE_LEVEL_METHOD, params, result);
}

bool
Policy::allowCaching(const Context *ctx, const TaggedBlock *block) {
    MessageParam<const Context> ctx_param(ctx);
    MessageParam<const TaggedBlock> block_param(block);
    
    MessageParamBase* param_list[2];
    param_list[0] = &ctx_param;
    param_list[1] = &block_param;
    
    MessageParams params(2, param_list);
    MessageResult<bool> result;
    
    MessageProcessor::instance()->process(ALLOW_CACHING_METHOD, params, result);
    return result.get();
}

bool
Policy::allowCachingInputCookie(const char *name) {
    MessageParam<const char> name_param(name);
    
    MessageParamBase* param_list[1];
    param_list[0] = &name_param;
    
    MessageParams params(1, param_list);
    MessageResult<bool> result;
    
    MessageProcessor::instance()->process(ALLOW_CACHING_INPUT_COOKIE_METHOD, params, result);
    return result.get();
}

bool
Policy::allowCachingOutputCookie(const char *name) {
    MessageParam<const char> name_param(name);
    
    MessageParamBase* param_list[1];
    param_list[0] = &name_param;
    
    MessageParams params(1, param_list);
    MessageResult<bool> result;
    
    MessageProcessor::instance()->process(ALLOW_CACHING_OUTPUT_COOKIE_METHOD, params, result);
    return result.get();
}

bool
Policy::isErrorDoc(xmlDocPtr doc) {
    MessageParam<xmlDocPtr> doc_param(&doc);
    
    MessageParamBase* param_list[1];
    param_list[0] = &doc_param;
    
    MessageParams params(1, param_list);
    MessageResult<bool> result;
    
    MessageProcessor::instance()->process(IS_ERROR_DOC_METHOD, params, result);
    return result.get();
}

namespace PolicyHandlers {

class RealIPHeaderNameHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        (void)params;
        result.set(&StringUtils::EMPTY_STRING);
        return 0;
    }
};

class GetPathBySchemeHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        const Request* request = params.getPtr<const Request>(0);
        const std::string& url = params.get<const std::string>(1);

        const char file_scheme[] = "file://";
        const char root_scheme[] = "docroot://";
        
        if (strncasecmp(url.c_str(), file_scheme, sizeof(file_scheme) - 1) == 0) {
            result.set(url.substr(sizeof(file_scheme) - 1));
        }
        else if (strncasecmp(url.c_str(), root_scheme, sizeof(root_scheme) - 1) == 0) {
            std::string::size_type pos = sizeof(root_scheme) - 1;
            if ('/' != url[pos]) {
                --pos;
            }
            result.set(VirtualHostData::instance()->getDocumentRoot(request) + (url.c_str() + pos));
        }
        else {
            result.set(url);
        }
        
        return 0;
    }
};

class GetRootBySchemeHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        const Request* request = params.getPtr<const Request>(0);
        const std::string& url = params.get<const std::string>(1);

        const char file_scheme[] = "file://";
        const char root_scheme[] = "docroot://";
        const char http_scheme[] = "http://";
        const char https_scheme[] = "https://";

        if ((strncasecmp(url.c_str(), file_scheme, sizeof(file_scheme) - 1) == 0) ||
            (strncasecmp(url.c_str(), root_scheme, sizeof(root_scheme) - 1) == 0) ||
            (strncasecmp(url.c_str(), http_scheme, sizeof(http_scheme) - 1) == 0) ||
            (strncasecmp(url.c_str(), https_scheme, sizeof(https_scheme) - 1) == 0)) {
            result.set(VirtualHostData::instance()->getDocumentRoot(request));
        }
        else {
            result.set(StringUtils::EMPTY_STRING);
        }
        
        return 0;
    }
};

class GetKeyHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        const std::string& name = params.get<const std::string>(1);
        result.set(name);
        return 0;
    }
};

class GetOutputEncodingHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        (void)params;
        result.set(Policy::UTF8_ENCODING);
        return 0;
    }
};

class IsSkippedProxyHeaderHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        const std::string& header = params.get<const std::string>(0);
        result.set(ProxyHeadersHelper::skipped(header.c_str()));
        return 0;
    }
};

class AllowCachingHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        (void)params;
        result.set(true);
        return 0;
    }
};

class AllowCachingInputCookieHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        (void)params;
        result.set(true);
        return 0;
    }
};

class AllowCachingOutputCookieHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        (void)params;
        result.set(false);
        return 0;
    }
};

class IsErrorDocHandler : public MessageHandler {
    int process(const MessageParams &params, MessageResultBase &result) {
        (void)params;
        result.set(false);
        return 0;
    }
};

struct HandlerRegisterer {
    HandlerRegisterer() {
        MessageProcessor::instance()->registerBack(Policy::REAL_IP_HEADER_NAME_METHOD,
                boost::shared_ptr<MessageHandler>(new RealIPHeaderNameHandler()));
        MessageProcessor::instance()->registerBack(Policy::GET_PATH_BY_SCHEME_METHOD,
                boost::shared_ptr<MessageHandler>(new GetPathBySchemeHandler()));
        MessageProcessor::instance()->registerBack(Policy::GET_ROOT_BY_SCHEME_METHOD,
                boost::shared_ptr<MessageHandler>(new GetRootBySchemeHandler()));
        MessageProcessor::instance()->registerBack(Policy::GET_KEY_METHOD,
                boost::shared_ptr<MessageHandler>(new GetKeyHandler()));
        MessageProcessor::instance()->registerBack(Policy::GET_OUTPUT_ENCODING_METHOD,
                boost::shared_ptr<MessageHandler>(new GetOutputEncodingHandler()));
        MessageProcessor::instance()->registerBack(Policy::USE_DEFAULT_SANITIZER_METHOD,
                boost::shared_ptr<MessageHandler>(new MessageHandler()));
        MessageProcessor::instance()->registerBack(Policy::IS_SKIPPED_PROXY_HEADER_METHOD,
                boost::shared_ptr<MessageHandler>(new IsSkippedProxyHeaderHandler()));
        MessageProcessor::instance()->registerBack(Policy::PROCESS_CACHE_LEVEL_METHOD,
                boost::shared_ptr<MessageHandler>(new MessageHandler()));
        MessageProcessor::instance()->registerBack(Policy::ALLOW_CACHING_METHOD,
                boost::shared_ptr<MessageHandler>(new AllowCachingHandler()));
        MessageProcessor::instance()->registerBack(Policy::ALLOW_CACHING_INPUT_COOKIE_METHOD,
                boost::shared_ptr<MessageHandler>(new AllowCachingInputCookieHandler()));
        MessageProcessor::instance()->registerBack(Policy::ALLOW_CACHING_OUTPUT_COOKIE_METHOD,
                boost::shared_ptr<MessageHandler>(new AllowCachingOutputCookieHandler()));
        MessageProcessor::instance()->registerBack(Policy::IS_ERROR_DOC_METHOD,
                boost::shared_ptr<MessageHandler>(new IsErrorDocHandler()));
    }
};

static HandlerRegisterer reg_handlers;

} // namespace PolicyHandlers
} // namespace xscript
