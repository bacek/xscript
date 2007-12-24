#include "settings.h"

#include <queue>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/current_function.hpp>

#include "xscript/util.h"
#include "xscript/config.h"
#include "xscript/logger.h"
#include "xscript/thread_pool.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

class StandardThreadPool : public ThreadPool, private boost::thread_group
{
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
	
	boost::mutex mutex_;
	boost::condition condition_;
};

StandardThreadPool::StandardThreadPool() : 
	running_(true)
{
}

StandardThreadPool::~StandardThreadPool() {
	stop();
}

void
StandardThreadPool::init(const Config *config) {
	
	try {
		int nthreads = config->as<unsigned short>("/xscript/pool-workers");
		boost::function<void()> f = boost::bind(&StandardThreadPool::handle, this);
		for (int i = 0; i < nthreads; ++i) {
			create_thread(f);
		}
	}
	catch (const std::exception &e) {
		stop();
		throw;
	}
}

void
StandardThreadPool::invoke(boost::function<void()> f) {
	boost::mutex::scoped_lock sl(mutex_);
	if (running_) {
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
	condition_.wait(sl, boost::bind(&StandardThreadPool::canRun, this));
	if (running_) {
		f = tasks_.front();
		tasks_.pop();
	}
	return f;
}

static ComponentRegisterer<ThreadPool> reg_(new StandardThreadPool());

} // namespace xscript
