#ifndef _XSCRIPT_MIST_BLOCK_H_
#define _XSCRIPT_MIST_BLOCK_H_

#include <memory>

#include "xscript/block.h"
#include "xscript/extension.h"

#include "mist_worker.h"

namespace xscript {

class MistBlock : public Block {
public:
    MistBlock(const Extension *ext, Xml *owner, xmlNodePtr node);
    virtual ~MistBlock();

protected:
    virtual void postParse();
    virtual void call(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const throw (std::exception);

private:
    MistBlock(const MistBlock &);
    MistBlock& operator = (const MistBlock &);

private:
    std::auto_ptr<MistWorker> worker_;
};

class MistExtension : public Extension {
public:
    MistExtension();
    virtual ~MistExtension();

    virtual const char* name() const;
    virtual const char* nsref() const;

    virtual void initContext(Context *ctx);
    virtual void stopContext(Context *ctx);
    virtual void destroyContext(Context *ctx);

    virtual std::auto_ptr<Block> createBlock(Xml *owner, xmlNodePtr node);
    virtual void init(const Config *config);

private:
    MistExtension(const MistExtension &);
    MistExtension& operator = (const MistExtension &);
};

} // namespace xscript

#endif // _XSCRIPT_MIST_BLOCK_H_
