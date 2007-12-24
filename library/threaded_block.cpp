#include "settings.h"

#include <boost/lexical_cast.hpp>

#include "xscript/threaded_block.h"
#include "xscript/script.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

ThreadedBlock::ThreadedBlock(Xml *owner, xmlNodePtr node) :
	Block(owner, node), threaded_(false), timeout_(5000), remote_timeout_(5000)
{
}

ThreadedBlock::~ThreadedBlock() {
}

int
ThreadedBlock::timeout() const {
	return timeout_;
}

int
ThreadedBlock::remoteTimeout() const {
	return remote_timeout_;
}

bool
ThreadedBlock::threaded() const {
	return threaded_;
}

void
ThreadedBlock::threaded(bool value) {
	threaded_ = value;
}

void
ThreadedBlock::property(const char *name, const char *value) {
	if (strncasecmp(name, "threaded", sizeof("threaded")) == 0) {
		threaded_ = (strncasecmp(value, "yes", sizeof("yes")) == 0);
	}
	else if (strncasecmp(name, "timeout", sizeof("timeout")) == 0) {
		timeout_ = boost::lexical_cast<int>(value);
	}
	else if (strncasecmp(name, "remote-timeout", sizeof("remote-timeout")) == 0) {
		remote_timeout_ = boost::lexical_cast<int>(value);
	}
	else {
		Block::property(name, value);
	}
}

} // namespace xscript
