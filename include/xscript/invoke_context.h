#ifndef _XSCRIPT_INVOKE_CONTEXT_H_
#define _XSCRIPT_INVOKE_CONTEXT_H_

#include <boost/noncopyable.hpp>

#include "xscript/xml_helpers.h"

namespace xscript {

class Context;
class Tag;

class InvokeContext : private boost::noncopyable {
public:
    InvokeContext();
    virtual ~InvokeContext();
    
    enum ResultType {
        ERROR  = 1,
        NO_CACHE = 2,
        SUCCESS  = 3,
    };
    
    XmlDocSharedHelper resultDoc() const;
    ResultType resultType() const;
    const Tag& tag() const;
    bool tagged() const;
    bool haveCachedCopy() const;
    void haveCachedCopy(bool flag);
    
    void resultDoc(const XmlDocSharedHelper &doc);
    void resultDoc(XmlDocHelper doc);
    void resultType(ResultType type);
    void tag(const Tag &tag);
    void resetTag();
     
    bool error() const;
    bool success() const;
    bool noCache() const;
    
    void setLocalContext(const boost::shared_ptr<Context> &ctx);
    const boost::shared_ptr<Context>& getLocalContext();

private:
    struct ContextData;
    ContextData *ctx_data_;
};

} // namespace xscript

#endif // _XSCRIPT_INVOKE_CONTEXT_H_
