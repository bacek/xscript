#ifndef _XSCRIPT_REFRESHER_H_
#define _XSCRIPT_REFRESHER_H_

#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>

namespace xscript {

class Refresher : private boost::noncopyable {
public:
    Refresher(boost::function<void()> f, boost::uint32_t timeout);
    virtual ~Refresher();

private:
    void refreshingThread();

private:
    boost::function<void()> f_;
    boost::uint32_t timeout_;
    volatile bool stopping_;
    boost::condition condition_;
    boost::mutex mutex_;
    boost::thread refreshingThread_;
};

} // namespace xscript

#endif // _XSCRIPT_REFRESHER_H_

