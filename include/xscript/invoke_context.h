#ifndef _XSCRIPT_INVOKE_CONTEXT_H_
#define _XSCRIPT_INVOKE_CONTEXT_H_

#include <boost/noncopyable.hpp>

#include "xscript/xml_helpers.h"

namespace xscript {

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
        NO_CACHE = 2,
        SUCCESS  = 3,
    };
    
    InvokeContext* parent(Context *ctx) const;

    XmlDocSharedHelper resultDoc() const;
    XmlDocSharedHelper metaDoc() const;
    xmlDocPtr resultDocPtr() const;
    xmlDocPtr metaDocPtr() const;
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
    bool success() const;
    bool noCache() const;
    TagKey* tagKey() const;
    
    void setLocalContext(const boost::shared_ptr<Context> &ctx);
    const boost::shared_ptr<Context>& getLocalContext();

    Meta* meta() const;
    void setMeta(const std::string &name, const std::string &value);
    void setMetaFlag();

private:
    struct ContextData;
    ContextData *ctx_data_;
};

} // namespace xscript

#endif // _XSCRIPT_INVOKE_CONTEXT_H_
