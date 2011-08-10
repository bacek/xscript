#ifndef _XSCRIPT_RENAMED_BLOCK_H_
#define _XSCRIPT_RENAMED_BLOCK_H_

#include <string>
#include <xscript/block.h>

namespace xscript {

class RenamedBlock : public Block {
public:
    RenamedBlock(const Extension *ext, Xml *owner, xmlNodePtr node);
    virtual ~RenamedBlock();

protected:
    virtual void property(const char *name, const char *value);

    virtual void wrapInvokeContext(InvokeContext &invoke_ctx) const throw (std::exception);

    class CallResultWrapper {
    public:
        CallResultWrapper(const RenamedBlock &block, InvokeContext &ctx) : block_(block), ctx_(ctx) {}
        ~CallResultWrapper() { block_.wrapInvokeContext(ctx_); }

    private:
        CallResultWrapper(const CallResultWrapper &);
        CallResultWrapper& operator = (const CallResultWrapper &);

        const RenamedBlock &block_;
	InvokeContext &ctx_;
    };

private:
    RenamedBlock(const RenamedBlock &);
    RenamedBlock& operator = (const RenamedBlock &);
        
private:
    std::string root_name_;
    xmlNsPtr root_ns_;
};

} // namespace xscript

#endif // _XSCRIPT_RENAMED_BLOCK_H_

