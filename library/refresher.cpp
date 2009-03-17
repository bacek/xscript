#include "settings.h"

#include <boost/bind.hpp>

#include "xscript/refresher.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

Refresher::Refresher(boost::function<void()> f, boost::uint32_t timeout) :
    f_(f),
    timeout_(timeout),
    stopping_(false),
    refreshingThread_(boost::bind(&Refresher::refreshingThread, this)) {
}

Refresher::~Refresher() {
    stopping_ = true;
    condition_.notify_one();
    refreshingThread_.join();
}

void
Refresher::refreshingThread() {

    while (!stopping_) {
        boost::mutex::scoped_lock lock(mutex_);
        boost::xtime t;
        boost::xtime_get(&t, boost::TIME_UTC);
        t.sec += timeout_;
        condition_.timed_wait(lock, t);
        if (!stopping_) {
            f_();
        }
    }
}

} // namespace xscript

