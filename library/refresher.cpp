#include "settings.h"

#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>

#include "xscript/refresher.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

class Refresher::RefresherData {
public:
    RefresherData(boost::function<void()> f, boost::uint32_t timeout) :
        f_(f),
        timeout_(timeout),
        stopping_(false),
        refreshingThread_(boost::bind(&RefresherData::refreshingThread, this))
    {}
    ~RefresherData() {
        stopping_ = true;
        condition_.notify_one();
        refreshingThread_.join();
    }
    
    void refreshingThread() {
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
    
    boost::function<void()> f_;
    boost::uint32_t timeout_;
    volatile bool stopping_;
    boost::condition condition_;
    boost::mutex mutex_;
    boost::thread refreshingThread_;
};

Refresher::Refresher(boost::function<void()> f, boost::uint32_t timeout) :
    data_(new RefresherData(f, timeout)) {
}

Refresher::~Refresher() {
}

} // namespace xscript

