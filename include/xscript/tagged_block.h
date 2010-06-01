#ifndef _XSCRIPT_TAGGED_BLOCK_H_
#define _XSCRIPT_TAGGED_BLOCK_H_

#include <ctime>

#include <xscript/block.h>
#include <xscript/cached_object.h>
#include <xscript/functors.h>
#include <xscript/tag.h>
#include <xscript/xml_helpers.h>

namespace xscript {

class TaggedBlock : public virtual Block, public CachedObject {
public:
    TaggedBlock(const Extension *ext, Xml *owner, xmlNodePtr node);
    virtual ~TaggedBlock();

    virtual std::string canonicalMethod(const Context *ctx) const;
    void createCanonicalMethod(const char* prefix);

    virtual bool tagged() const;
    virtual void tagged(bool tagged);
    
    virtual bool remote() const;
    
    virtual time_t cacheTime() const;
    virtual void cacheTime(time_t cache_time);
    
    virtual std::string info(const Context *ctx) const;

    virtual std::string createTagKey(const Context *ctx) const;

    void cacheLevel(unsigned char type, bool value);
    bool cacheLevel(unsigned char type) const;
    
protected:
    bool cacheTimeUndefined() const;
    virtual void invokeInternal(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx);
    virtual void postCall(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx);
    virtual void postParse();
    virtual void property(const char *name, const char *value);
    bool propertyInternal(const char *name, const char *value);
    int tagPosition() const;
    bool haveTagParam() const;
    
    void parseParamNode(const xmlNodePtr node);
    
    std::string processMainKey(const Context *ctx) const;
    std::string processParamsKey(const Context *ctx) const;

    void callMetaCacheLua(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx);

private:
    struct TaggedBlockData;
    TaggedBlockData *tb_data_;
};

} // namespace xscript

#endif // _XSCRIPT_TAGGED_BLOCK_H_
