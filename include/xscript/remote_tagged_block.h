#ifndef _XSCRIPT_REMOTE_TAGGED_BLOCK_H_
#define _XSCRIPT_REMOTE_TAGGED_BLOCK_H_

#include <xscript/context.h>
#include <xscript/threaded_block.h>
#include <xscript/tagged_block.h>
#include <xscript/xml.h>

namespace xscript {

class RemoteTaggedBlock : public ThreadedBlock, public TaggedBlock {
public:
    RemoteTaggedBlock(const Extension *ext, Xml *owner, xmlNodePtr node);
    virtual ~RemoteTaggedBlock();

    virtual void call(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const throw (std::exception);
    
    int remoteTimeout() const;
    void remoteTimeout(int timeout);
    bool isDefaultRemoteTimeout() const;
    void setDefaultRemoteTimeout();

    virtual bool remote() const;
    
    virtual int invokeTimeout() const;
    unsigned int retryCount() const;

protected:
    virtual void property(const char *name, const char *value);
    virtual void retryCall(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const throw (std::exception) = 0;
    virtual void postParse();
    virtual int remainedTime(Context *ctx) const;

private:
    struct RemoteTaggedBlockData;
    RemoteTaggedBlockData *rtb_data_;
};

class RetryInvokeError : public InvokeError {
public:
    RetryInvokeError(const std::string &error) : InvokeError(error) {}
    RetryInvokeError(const std::string &error, XmlNodeHelper node) :
        InvokeError(error, node) {}
    RetryInvokeError(const std::string &error, const std::string &name, const std::string &value) :
        InvokeError(error, name, value) {}
};

} // namespace xscript

#endif // _XSCRIPT_REMOTE_TAGGED_BLOCK_H_
