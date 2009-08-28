#ifndef _XSCRIPT_POLICY_H_
#define _XSCRIPT_POLICY_H_

#include <string>
#include <vector>

#include <libxml/tree.h>

#include "xscript/functors.h"

namespace xscript {

class Context;
class Request;
class TaggedBlock;

class Policy {
public:
    static const std::string& realIPHeaderName();
    static std::string getPathByScheme(const Request *request, const std::string &url);
    static std::string getRootByScheme(const Request *request, const std::string &url);

    static std::string getKey(const Request* request, const std::string& name);
    static std::string getOutputEncoding(const Request* request);

    static void useDefaultSanitizer();
    static void processCacheLevel(TaggedBlock *block, const std::string &no_cache);
    static bool allowCaching(const Context *ctx, const TaggedBlock *block);
    static bool allowCachingInputCookie(const char *name);
    static bool allowCachingOutputCookie(const char *name);
    static bool isSkippedProxyHeader(const std::string &header);
    
    static bool isErrorDoc(xmlDocPtr doc);
    static std::string getCacheCookie(const Context *ctx, const std::string &cookie);
    
    static const std::string REAL_IP_HEADER_NAME_METHOD;
    static const std::string GET_PATH_BY_SCHEME_METHOD;
    static const std::string GET_ROOT_BY_SCHEME_METHOD;
    static const std::string GET_KEY_METHOD;
    static const std::string GET_OUTPUT_ENCODING_METHOD;
    static const std::string USE_DEFAULT_SANITIZER_METHOD;
    static const std::string PROCESS_CACHE_LEVEL_METHOD;
    static const std::string ALLOW_CACHING_METHOD;
    static const std::string ALLOW_CACHING_INPUT_COOKIE_METHOD;
    static const std::string ALLOW_CACHING_OUTPUT_COOKIE_METHOD;
    static const std::string IS_SKIPPED_PROXY_HEADER_METHOD;
    static const std::string IS_ERROR_DOC_METHOD;
    static const std::string GET_CACHE_COOKIE_METHOD;
    
    static const std::string UTF8_ENCODING;
};

} // namespace xscript

#endif // _XSCRIPT_POLICY_H_
