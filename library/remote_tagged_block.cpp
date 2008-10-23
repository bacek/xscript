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
        Block(ext, owner, node), ThreadedBlock(ext, owner, node), TaggedBlock(ext, owner, node) {
    setDefaultRemoteTimeout();
}

RemoteTaggedBlock::~RemoteTaggedBlock() {
}

void
RemoteTaggedBlock::property(const char *name, const char *value) {
    if (strncasecmp(name, "remote-timeout", sizeof("remote-timeout")) == 0) {
        remote_timeout_ = boost::lexical_cast<int>(value);
    }
    else {
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
    return remoteTimeout();
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

    if (!tagged() && !isDefaultRemoteTimeout()) {
        if (OperationMode::instance()->isProduction()) {
            setDefaultRemoteTimeout();
            log()->warn("%s, remote timeout setup is prohibited for non-tagged blocks or when tag cache time is nil: %s",
                        BOOST_CURRENT_FUNCTION, owner()->name().c_str());
        }
        else {
            throw std::runtime_error("remote timeout setup is prohibited for non-tagged blocks or when tag cache time is nil");
        }
    }
}

} // namespace xscript
