#ifndef _XSCRIPT_TAG_PARAM_H_
#define _XSCRIPT_TAG_PARAM_H_

#include "xscript/param.h"

namespace xscript {

class TaggedBlock;

class TagParam : public Param {
public:
    TagParam(TaggedBlock *owner, xmlNodePtr node);
    virtual ~TagParam();

    virtual void parse();
    virtual std::string asString(const Context *ctx) const;
    virtual void add(const Context *ctx, ArgList &al) const;

    static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);

private:
    TaggedBlock *owner_;
};

}

#endif // _XSCRIPT_TAG_PARAM_H_
