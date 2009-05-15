#ifndef _XSCRIPT_POLICY_H_
#define _XSCRIPT_POLICY_H_

#include <string>
#include <vector>

#include "xscript/component.h"
#include "xscript/functors.h"

namespace xscript {

class Context;
class Request;
class TaggedBlock;

class Policy: public virtual Component<Policy> {
public:
    Policy();
    virtual ~Policy();

    virtual const std::string& realIPHeaderName() const;
    virtual std::string getPathByScheme(const Request *request, const std::string &url) const;
    virtual std::string getRootByScheme(const Request *request, const std::string &url) const;

    virtual std::string getKey(const Request* request, const std::string& name) const;
    virtual std::string getOutputEncoding(const Request* request) const;

    virtual void useDefaultSanitizer() const;
    virtual void processCacheLevel(TaggedBlock *block, const std::string &no_cache) const;
    virtual bool allowCaching(const Context *ctx, const TaggedBlock *block) const;
    virtual bool allowCachingCookie(const char *name) const;
    virtual bool isSkippedProxyHeader(const std::string &header) const;
    
private:
    static const std::string UTF8_ENCODING;
};

} // namespace xscript

#endif // _XSCRIPT_POLICY_H_
