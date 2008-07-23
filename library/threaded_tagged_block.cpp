#include "settings.h"

#include "xscript/checking_policy.h"
#include "xscript/logger.h"
#include "xscript/threaded_tagged_block.h"

#include <boost/current_function.hpp>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

ThreadedTaggedBlock::ThreadedTaggedBlock(const Extension *ext, Xml *owner, xmlNodePtr node) :
	Block(ext, owner, node), ThreadedBlock(ext, owner, node), TaggedBlock(ext, owner, node)
{
}

ThreadedTaggedBlock::~ThreadedTaggedBlock() {
}

void
ThreadedTaggedBlock::property(const char *name, const char *value) {
	ThreadedBlock::property(name, value);
}

void
ThreadedTaggedBlock::postParse() {
	ThreadedBlock::postParse();
	TaggedBlock::postParse();

	if ((!tagged() || cacheTime() == 0) &&
		(remoteTimeout() != timeout())) {
		if (CheckingPolicy::instance()->isProduction()) {
			remoteTimeout(timeout());
			log()->warn("%s, remote timeout setup is prohibited for non-tagged blocks or when tag cache time is nil: %s",
				BOOST_CURRENT_FUNCTION, owner()->name().c_str());
		}
		else {
			throw std::runtime_error("remote timeout setup is prohibited for non-tagged blocks or when tag cache time is nil");
		}
	}
}

} // namespace xscript
