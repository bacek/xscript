#ifndef _XSCRIPT_THREADED_BLOCK_H_
#define _XSCRIPT_THREADED_BLOCK_H_

#include <xscript/block.h>

namespace xscript {

class ThreadedBlock : public virtual Block {
public:
    ThreadedBlock(const Extension *ext, Xml* owner, xmlNodePtr node);
    virtual ~ThreadedBlock();

    int timeout() const;
    virtual int invokeTimeout() const;

    virtual bool threaded() const;
    virtual void threaded(bool value);

    virtual void startTimer(Context *ctx); //TODO: const
    virtual void stopTimer(Context *ctx); //TODO: const

protected:
    virtual void property(const char *name, const char *value);
    virtual void postInvoke(Context *ctx, InvokeContext *invoke_ctx);
    virtual int remainedTime(Context *ctx) const;

private:
    struct ThreadedBlockData;
    std::auto_ptr<ThreadedBlockData> trb_data_;
};

} // namespace xscript

#endif // _XSCRIPT_THREADED_BLOCK_H_
