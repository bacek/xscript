#include "settings.h"

#include "xscript/thread_pool.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

bool
ThreadPool::invokeEx(boost::function<void()> /* f_threaded */, boost::function<void()> f_unthreaded) {
    f_unthreaded();
    return false;
}

void
ThreadPool::stop() {
}

static ComponentRegisterer<ThreadPool> reg_;

} // namespace xscript
