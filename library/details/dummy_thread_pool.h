#ifndef _XSCRIPT_DETAILS_DUMMY_THREAD_POOL_H_
#define _XSCRIPT_DETAILS_DUMMY_THREAD_POOL_H_

#include "xscript/thread_pool.h"

namespace xscript {

class DummyThreadPool : public ThreadPool {
public:
    virtual void invoke(boost::function<void()> f);
    virtual void stop();
};


}

#endif

