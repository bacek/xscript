#include "settings.h"

#include <cstring>
#include <stdexcept>

#include <boost/current_function.hpp>
#include <boost/lexical_cast.hpp>

#include "xscript/operation_mode.h"
#include "xscript/logger.h"
#include "xscript/remote_tagged_block.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

struct RemoteTaggedBlock::RemoteTaggedBlockData {
    RemoteTaggedBlockData() : remote_timeout_(0), retry_count_(0)
    {}
    
    ~RemoteTaggedBlockData() {
    }
    
    int remote_timeout_;
    unsigned int retry_count_;
};

RemoteTaggedBlock::RemoteTaggedBlock(const Extension *ext, Xml *owner, xmlNodePtr node) :
        Block(ext, owner, node), ThreadedBlock(ext, owner, node), TaggedBlock(ext, owner, node),
        rtb_data_(new RemoteTaggedBlockData())
{}

RemoteTaggedBlock::~RemoteTaggedBlock() {
    delete rtb_data_;
}

void
RemoteTaggedBlock::property(const char *name, const char *value) {
    if (strncasecmp(name, "remote-timeout", sizeof("remote-timeout")) == 0) {
        try {
            rtb_data_->remote_timeout_ = boost::lexical_cast<int>(value);
        }
        catch(const boost::bad_lexical_cast &e) {
            throw std::runtime_error(
                std::string("cannot parse remote-timeout value: ") + value);
        }
    }
    else if (strncasecmp(name, "retry-count", sizeof("retry-count")) == 0) {
        try {
            rtb_data_->retry_count_ = boost::lexical_cast<unsigned int>(value);
        }
        catch(const boost::bad_lexical_cast &e) {
            throw std::runtime_error(
                std::string("cannot parse retry-count value: ") + value);
        }
    }
    else if (!TaggedBlock::propertyInternal(name, value)) {
        ThreadedBlock::property(name, value);
    }
}

int
RemoteTaggedBlock::remoteTimeout() const {
    return rtb_data_->remote_timeout_ > 0 ? rtb_data_->remote_timeout_ : timeout();
}

void
RemoteTaggedBlock::remoteTimeout(int timeout) {
    rtb_data_->remote_timeout_ = timeout;
}

int
RemoteTaggedBlock::invokeTimeout() const {
    return std::max(remoteTimeout(), timeout());
}

bool
RemoteTaggedBlock::isDefaultRemoteTimeout() const {
    return rtb_data_->remote_timeout_ == 0;
}

void
RemoteTaggedBlock::setDefaultRemoteTimeout() {
    rtb_data_->remote_timeout_ = 0;
}

bool
RemoteTaggedBlock::remote() const {
    return true;
}

void
RemoteTaggedBlock::postParse() {
    ThreadedBlock::postParse();
    TaggedBlock::postParse();

    OperationMode::checkRemoteTimeout(this);
}

unsigned int
RemoteTaggedBlock::retryCount() const {
    return rtb_data_->retry_count_;
}

XmlDocHelper
RemoteTaggedBlock::call(boost::shared_ptr<Context> ctx,
    boost::shared_ptr<InvokeContext> invoke_ctx) throw (std::exception) {
    
    for(int rcount = retryCount(); rcount >= 0; --rcount) {
        try {
            if (ctx->stopBlocks()) {
                throw SkipResultInvokeError("block is stopped");
            }
            return retryCall(ctx, invoke_ctx);
        }
        catch(const RetryInvokeError &e) {
            if (rcount > 0) {
                log()->error("%s", e.what_info().c_str());
                continue;
            }
            throw e;
        }
    }
    throw std::runtime_error("Incorrect retry count parameter value");
}

int
RemoteTaggedBlock::remainedTime(Context *ctx) const {
    return std::min(ThreadedBlock::remainedTime(ctx), remoteTimeout());
}

} // namespace xscript
