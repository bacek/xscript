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

    virtual XmlDocHelper call(boost::shared_ptr<Context> ctx, boost::any &a) throw (std::exception);
    
    int remoteTimeout() const;
    void remoteTimeout(int timeout);
    bool isDefaultRemoteTimeout() const;
    void setDefaultRemoteTimeout();

    virtual int invokeTimeout() const;
    unsigned int retryCount() const;

protected:
    virtual void property(const char *name, const char *value);
    virtual XmlDocHelper retryCall(boost::shared_ptr<Context> ctx, boost::any &a) throw (std::exception) = 0;
    virtual void postParse();

private:
    int remote_timeout_;
    unsigned int retry_count_;
};

class RetryInvokeError : public InvokeError {
public:
    RetryInvokeError(const std::string &error) : InvokeError(error) {}
    RetryInvokeError(const std::string &error, const std::string &name, const std::string &value) :
        InvokeError(error, name, value) {}
};

} // namespace xscript

#endif // _XSCRIPT_REMOTE_TAGGED_BLOCK_H_
