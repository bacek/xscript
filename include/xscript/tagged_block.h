#ifndef _XSCRIPT_TAGGED_BLOCK_H_
#define _XSCRIPT_TAGGED_BLOCK_H_

#include <ctime>
#include <xscript/tag.h>
#include <xscript/block.h>
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

protected:
    virtual XmlDocHelper invokeInternal(Context *ctx);
    virtual void postCall(Context *ctx, const XmlDocHelper &doc, const boost::any &a);

private:
    std::string canonical_method_;
    bool tagged_;
    time_t cache_time_;
};

} // namespace xscript

#endif // _XSCRIPT_TAGGED_BLOCK_H_
