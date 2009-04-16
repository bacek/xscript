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

RemoteTaggedBlock::RemoteTaggedBlock(const Extension *ext, Xml *owner, xmlNodePtr node) :
        Block(ext, owner, node), ThreadedBlock(ext, owner, node), TaggedBlock(ext, owner, node), retry_count_(0) {
    setDefaultRemoteTimeout();
}

RemoteTaggedBlock::~RemoteTaggedBlock() {
}

void
RemoteTaggedBlock::property(const char *name, const char *value) {
    if (strncasecmp(name, "remote-timeout", sizeof("remote-timeout")) == 0) {
        remote_timeout_ = boost::lexical_cast<int>(value);
    }
    else if (strncasecmp(name, "retry-count", sizeof("retry-count")) == 0) {
        retry_count_ = boost::lexical_cast<unsigned int>(value);
    }
    else if (!TaggedBlock::propertyInternal(name, value)) {
        ThreadedBlock::property(name, value);
    }
}

int
RemoteTaggedBlock::remoteTimeout() const {
    return remote_timeout_ > 0 ? remote_timeout_ : timeout();
}

void
RemoteTaggedBlock::remoteTimeout(int timeout) {
    remote_timeout_ = timeout;
}

int
RemoteTaggedBlock::invokeTimeout() const {
    return std::max(remoteTimeout(), timeout());
}

bool
RemoteTaggedBlock::isDefaultRemoteTimeout() const {
    return remote_timeout_ == 0;
}

void
RemoteTaggedBlock::setDefaultRemoteTimeout() {
    remote_timeout_ = 0;
}

void
RemoteTaggedBlock::postParse() {
    ThreadedBlock::postParse();
    TaggedBlock::postParse();

    OperationMode::instance()->checkRemoteTimeout(this);
}

unsigned int
RemoteTaggedBlock::retryCount() const {
    return retry_count_;
}

XmlDocHelper
RemoteTaggedBlock::call(boost::shared_ptr<Context> ctx, boost::any &a) throw (std::exception) {
    for(int rcount = retryCount(); rcount >= 0; --rcount) {
        try {
            return retryCall(ctx, a);
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

} // namespace xscript
