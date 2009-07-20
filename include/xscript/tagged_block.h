#ifndef _XSCRIPT_TAGGED_BLOCK_H_
#define _XSCRIPT_TAGGED_BLOCK_H_

#include <ctime>

#include <xscript/block.h>
#include <xscript/functors.h>
#include <xscript/tag.h>
#include <xscript/xml_helpers.h>

namespace xscript {

class TaggedBlock : public virtual Block {
public:
    TaggedBlock(const Extension *ext, Xml *owner, xmlNodePtr node);
    virtual ~TaggedBlock();

    virtual std::string canonicalMethod(const Context *ctx) const;
    void createCanonicalMethod(const char* prefix);

    virtual bool tagged() const;
    virtual void tagged(bool tagged);
    
    virtual time_t cacheTime() const;
    virtual void cacheTime(time_t cache_time);
    
    virtual std::string info(const Context *ctx) const;

    virtual std::string createTagKey(const Context *ctx) const;

    void cacheLevel(unsigned char type, bool value);
    bool cacheLevel(unsigned char type) const;
    
protected:
    virtual void processParam(std::auto_ptr<Param> p);
    virtual InvokeResult invokeInternal(boost::shared_ptr<Context> ctx);
    virtual void postCall(Context *ctx, const InvokeResult &result, const boost::any &a);
    virtual void postParse();
    virtual void property(const char *name, const char *value);
    bool propertyInternal(const char *name, const char *value);
    int tagPosition() const;
    bool haveTagParam() const;
    bool cacheTimeUndefined() const;
    
    std::string processMainKey(const Context *ctx) const;
    std::string processParamsKey(const Context *ctx) const;

private:
    struct TaggedBlockData;
    TaggedBlockData *tb_data_;
};

} // namespace xscript

#endif // _XSCRIPT_TAGGED_BLOCK_H_
