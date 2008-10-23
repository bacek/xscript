#ifndef _XSCRIPT_REMOTE_TAGGED_BLOCK_H_
#define _XSCRIPT_REMOTE_TAGGED_BLOCK_H_

#include <xscript/threaded_block.h>
#include <xscript/tagged_block.h>
#include <xscript/xml.h>

namespace xscript {

class RemoteTaggedBlock : public ThreadedBlock, public TaggedBlock {
public:
    RemoteTaggedBlock(const Extension *ext, Xml *owner, xmlNodePtr node);
    virtual ~RemoteTaggedBlock();

    int remoteTimeout() const;
    void remoteTimeout(int timeout);
    bool isDefaultRemoteTimeout() const;
    void setDefaultRemoteTimeout();

    virtual int invokeTimeout() const;
protected:
    virtual void property(const char *name, const char *value);
    virtual void postParse();

private:
    int remote_timeout_;
};

} // namespace xscript

#endif // _XSCRIPT_REMOTE_TAGGED_BLOCK_H_
