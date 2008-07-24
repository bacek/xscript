#include "settings.h"

#include <stdexcept>

#include <boost/current_function.hpp>
#include <boost/lexical_cast.hpp>

#include "xscript/checking_policy.h"
#include "xscript/logger.h"
#include "xscript/threaded_tagged_block.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

ThreadedTaggedBlock::ThreadedTaggedBlock(const Extension *ext, Xml *owner, xmlNodePtr node) :
	Block(ext, owner, node), ThreadedBlock(ext, owner, node), TaggedBlock(ext, owner, node)
{
	setDefaultRemoteTimeout();
}

ThreadedTaggedBlock::~ThreadedTaggedBlock() {
}

void
ThreadedTaggedBlock::property(const char *name, const char *value) {
	if (strncasecmp(name, "remote-timeout", sizeof("remote-timeout")) == 0) {
		remote_timeout_ = boost::lexical_cast<int>(value);
	}
	else {
		ThreadedBlock::property(name, value);
	}
}


int
ThreadedTaggedBlock::remoteTimeout() const {
	return remote_timeout_ > 0 ? remote_timeout_ : timeout();
}

void
ThreadedTaggedBlock::remoteTimeout(int timeout) {
	remote_timeout_ = timeout;
}

bool
ThreadedTaggedBlock::isDefaultRemoteTimeout() const {
	return remote_timeout_ == 0;
}

void
ThreadedTaggedBlock::setDefaultRemoteTimeout() {
	remote_timeout_ = 0;
}

void
ThreadedTaggedBlock::postParse() {
	if ((!tagged() || cacheTime() == 0) && !isDefaultRemoteTimeout()) {
		if (CheckingPolicy::instance()->isProduction()) {
			setDefaultRemoteTimeout();
			log()->warn("%s, remote timeout setup is prohibited for non-tagged blocks or when tag cache time is nil: %s",
				BOOST_CURRENT_FUNCTION, owner()->name().c_str());
		}
		else {
			throw std::runtime_error("remote timeout setup is prohibited for non-tagged blocks or when tag cache time is nil");
		}
	}

	ThreadedBlock::postParse();
	TaggedBlock::postParse();
}

} // namespace xscript
