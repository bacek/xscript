#ifndef _XSCRIPT_WHILE_BLOCK_H_
#define _XSCRIPT_WHILE_BLOCK_H_

#include "local_block.h"

namespace xscript {

class Script;

class WhileBlock : public LocalBlock {
public:
    WhileBlock(const Extension *ext, Xml *owner, xmlNodePtr node);
    virtual ~WhileBlock();

protected:
    virtual void postParse();

private:
    WhileBlock(const WhileBlock &);
    WhileBlock& operator = (const WhileBlock &);

    virtual void property(const char *name, const char *value);
    virtual XmlDocHelper call(boost::shared_ptr<Context> ctx,
        boost::shared_ptr<InvokeContext> invoke_ctx) throw (std::exception);
};

} // namespace xscript

#endif // _XSCRIPT_WHILE_BLOCK_H_
