#include "settings.h"

#include "xscript/thread_pool.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

ThreadPool::ThreadPool() 
{
}

ThreadPool::~ThreadPool() {
}

void
ThreadPool::init(const Config *config) {
}

void
ThreadPool::invoke(boost::function<void()> f) {
	f();
}

void
ThreadPool::stop() {
}

} // namespace xscript
