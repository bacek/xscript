#ifndef _XSCRIPT_POLICY_H_
#define _XSCRIPT_POLICY_H_

#include <string>
#include <vector>

#include <libxml/tree.h>

#include "xscript/component.h"

namespace xscript {

class Context;
class Request;
class TaggedBlock;

class Policy : public Component<Policy> {
public:
    Policy();
    virtual ~Policy();
    virtual const std::string& realIPHeaderName();
    virtual std::string getPathByScheme(const Request *request, const std::string &url);
    virtual std::string getRootByScheme(const Request *request, const std::string &url);
    virtual std::string getKey(const Request* request, const std::string& name);
    virtual std::string getOutputEncoding(const Request* request);
    virtual void useDefaultSanitizer();
    virtual void processCacheLevel(TaggedBlock *block, const std::string &no_cache);
    virtual bool allowCaching(const Context *ctx, const TaggedBlock *block);
    virtual bool allowCachingInputCookie(const char *name);
    virtual bool allowCachingOutputCookie(const char *name);
    virtual bool isSkippedProxyHeader(const std::string &header);
    virtual bool isErrorDoc(xmlDocPtr doc);
};

} // namespace xscript

#endif // _XSCRIPT_POLICY_H_
