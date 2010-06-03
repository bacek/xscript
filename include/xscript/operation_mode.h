#ifndef _XSCRIPT_OPERATION_MODE_H_
#define _XSCRIPT_OPERATION_MODE_H_

#include <string>

#include <xscript/component.h>

namespace xscript {

class Block;
class Context;
class InvokeError;
class RemoteTaggedBlock;
class Request;
class Response;
class Script;
class Stylesheet;

class OperationMode : public Component<OperationMode> {
public:
    OperationMode();
    virtual ~OperationMode();
    virtual void processError(const std::string& message);
    virtual void processCriticalInvokeError(const std::string& message);
    virtual void sendError(Response* response, unsigned short status, const std::string& message);
    virtual bool isProduction();
    virtual void assignBlockError(Context *ctx, const Block *block, const std::string &error);
    virtual void processPerblockXsltError(const Context *ctx, const Block *block);
    virtual void processScriptError(const Context *ctx, const Script *script);
    virtual void processMainXsltError(const Context *ctx, const Script *script, const Stylesheet *style);
    virtual void processXmlError(const std::string &filename);
    virtual void collectError(const InvokeError &error, InvokeError &full_error);
    virtual bool checkDevelopmentVariable(const Request* request, const std::string &var);
    virtual void checkRemoteTimeout(RemoteTaggedBlock *block);
};

} // namespace xscript

#endif //_XSCRIPT_OPERATION_MODE_H_

