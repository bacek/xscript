#include "settings.h"

#include "xscript/thread_pool.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

void
ThreadPool::invoke(boost::function<void()> f) {
    f();
}

void
ThreadPool::stop() {
}

static ComponentRegisterer<ThreadPool> reg_;

} // namespace xscript
