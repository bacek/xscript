#ifndef _XSCRIPT_OPERATION_MODE_H_
#define _XSCRIPT_OPERATION_MODE_H_

#include <string>

namespace xscript {

class Block;
class Context;
class InvokeError;
class RemoteTaggedBlock;
class Request;
class Response;
class Script;
class Stylesheet;

class OperationMode {
public:
    static void processError(const std::string& message);
    static void processCriticalInvokeError(const std::string& message);
    static void sendError(Response* response, unsigned short status, const std::string& message);
    static bool isProduction();
    static void assignBlockError(Context *ctx, const Block *block, const std::string &error);
    static void processPerblockXsltError(const Context *ctx, const Block *block);
    static void processScriptError(const Context *ctx, const Script *script);
    static void processMainXsltError(const Context *ctx, const Script *script, const Stylesheet *style);
    static void processXmlError(const std::string &filename);
    static void collectError(const InvokeError &error, InvokeError &full_error);
    static bool checkDevelopmentVariable(const Request* request, const std::string &var);
    static void checkRemoteTimeout(RemoteTaggedBlock *block);
    
    static const std::string PROCESS_ERROR_METHOD;
    static const std::string PROCESS_CRITICAL_INVOKE_ERROR_METHOD;
    static const std::string SEND_ERROR_METHOD;
    static const std::string IS_PRODUCTION_METHOD;
    static const std::string ASSIGN_BLOCK_ERROR_METHOD;
    static const std::string PROCESS_PERBLOCK_XSLT_ERROR_METHOD;
    static const std::string PROCESS_SCRIPT_ERROR_METHOD;
    static const std::string PROCESS_MAIN_XSLT_ERROR_METHOD;
    static const std::string PROCESS_XML_ERROR_METHOD;
    static const std::string COLLECT_ERROR_METHOD;
    static const std::string CHECK_DEVELOPMENT_VARIABLE_METHOD;
    static const std::string CHECK_REMOTE_TIMEOUT_METHOD;
};

} // namespace xscript

#endif //_XSCRIPT_OPERATION_MODE_H_

