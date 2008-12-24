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

    virtual const TimeoutCounter& timer() const;
    virtual void startTimer(const Context *ctx);

protected:
    virtual void property(const char *name, const char *value);
    bool propertyInternal(const char *name, const char *value);
    virtual void resetTimer(int timeout);

private:
    bool threaded_;
    int timeout_;
    TimeoutCounter timer_;
};

} // namespace xscript

#endif // _XSCRIPT_THREADED_BLOCK_H_
