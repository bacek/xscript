#include "settings.h"

#include <queue>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/current_function.hpp>

#include "xscript/util.h"
#include "xscript/config.h"
#include "xscript/logger.h"
#include "xscript/thread_pool.h"
#include "xscript/simple_counter.h"
#include "xscript/status_info.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class StandardThreadPool : public ThreadPool, private boost::thread_group {
public:
    StandardThreadPool();
    virtual ~StandardThreadPool();

    virtual void init(const Config *config);
    virtual void invoke(boost::function<void()> f);
    virtual void stop();

protected:

    void handle();
    bool canRun() const;
    boost::function<void()> wait();

private:
    typedef boost::function<void()> Task;

private:
    bool running_;
    std::queue<Task> tasks_;
    unsigned short free_threads_;

    boost::mutex mutex_;
    boost::condition condition_;

    std::auto_ptr<SimpleCounter> counter_;
};

StandardThreadPool::StandardThreadPool() : running_(true), free_threads_(0) {
}

StandardThreadPool::~StandardThreadPool() {
    stop();
}

void
StandardThreadPool::init(const Config *config) {

    try {
        counter_ = SimpleCounterFactory::instance()->createCounter("working-threads", true);
        unsigned short nthreads = config->as<unsigned short>("/xscript/pool-workers");
        counter_->max(nthreads);
        boost::function<void()> f = boost::bind(&StandardThreadPool::handle, this);
        for (int i = 0; i < nthreads; ++i) {
            create_thread(f);
        }
    }
    catch (const std::exception &e) {
        stop();
        throw;
    }

    StatusInfo::instance()->getStatBuilder().addCounter(counter_.get());
}

void
StandardThreadPool::invoke(boost::function<void()> f) {
    boost::mutex::scoped_lock sl(mutex_);
    if (running_) {
        if (0 == free_threads_) {
            f();
            return;
        }
        tasks_.push(f);
        condition_.notify_all();
    }
}

void
StandardThreadPool::stop() {
    boost::mutex::scoped_lock sl(mutex_);
    if (running_) {
        running_ = false;
        while (!tasks_.empty()) {
            tasks_.pop();
        }
        condition_.notify_all();
        sl.unlock();
        join_all();
    }
}

void
StandardThreadPool::handle() {
    while (true) {
        boost::function<void()> f = wait();
        if (f.empty()) {
            return;
        }
        SimpleCounter::ScopedCount c(counter_.get());
        f();
    }
}

bool
StandardThreadPool::canRun() const {
    return !tasks_.empty() || !running_;
}

boost::function<void()>
StandardThreadPool::wait() {
    boost::function<void()> f;
    boost::mutex::scoped_lock sl(mutex_);
    ++free_threads_;
    condition_.wait(sl, boost::bind(&StandardThreadPool::canRun, this));
    if (running_) {
        f = tasks_.front();
        tasks_.pop();
        --free_threads_;
    }
    return f;
}

static ComponentImplRegisterer<ThreadPool> reg_(new StandardThreadPool());

} // namespace xscript
