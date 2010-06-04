#ifndef _XSCRIPT_INVOKE_CONTEXT_H_
#define _XSCRIPT_INVOKE_CONTEXT_H_

#include <boost/noncopyable.hpp>

#include "xscript/xml_helpers.h"

namespace xscript {

class Block;
class Context;
class Meta;
class Tag;
class TagKey;

class InvokeContext : private boost::noncopyable {
public:
    InvokeContext();
    InvokeContext(InvokeContext *parent);
    virtual ~InvokeContext();
    
    enum ResultType {
        ERROR  = 1,
        META_ERROR  = 2,
        NO_CACHE = 3,
        SUCCESS  = 4,
    };
    
    InvokeContext* parent(Context *ctx) const;

    XmlDocSharedHelper resultDoc() const;
    XmlDocSharedHelper metaDoc() const;
    ResultType resultType() const;
    const Tag& tag() const;
    bool tagged() const;
    bool haveCachedCopy() const;
    void haveCachedCopy(bool flag);
    void tagKey(const boost::shared_ptr<TagKey> &key);
    
    void resultDoc(const XmlDocSharedHelper &doc);
    void resultDoc(XmlDocHelper doc);
    void metaDoc(const XmlDocSharedHelper &doc);
    void metaDoc(XmlDocHelper doc);
    void resultType(ResultType type);
    void tag(const Tag &tag);
    void resetTag();
     
    bool error() const;
    bool meta_error() const;
    bool success() const;
    bool noCache() const;
    TagKey* tagKey() const;
    
    void setLocalContext(const boost::shared_ptr<Context> &ctx);
    const boost::shared_ptr<Context>& getLocalContext();

    boost::shared_ptr<Meta> meta() const;
    void setMeta(const boost::shared_ptr<Meta> &meta);

private:
    struct ContextData;
    std::auto_ptr<ContextData> ctx_data_;
};

} // namespace xscript

#endif // _XSCRIPT_INVOKE_CONTEXT_H_
