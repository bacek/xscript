#ifndef _XSCRIPT_DAEMON_CLEANUP_MANAGER_H_
#define _XSCRIPT_DAEMON_CLEANUP_MANAGER_H_

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>

#include "xscript/context.h"

namespace xscript {

class CleanupManager {
public:
    CleanupManager();
    CleanupManager(unsigned int max_size);
    ~CleanupManager();
    
    void init(unsigned int max_size);
    void push(boost::shared_ptr<Context> ctx);
    
private:
    void commonWorker();
    void uniqueWorker();
    
    bool commonRun() const;
    bool uniqueRun() const;
    
    bool uniqueFull() const;
    
private:
    typedef std::list<boost::shared_ptr<Context> > ListType;
    
    volatile bool stopped_;
    unsigned int max_size_;
    ListType common_;
    ListType unique_;
    std::auto_ptr<boost::thread> commonThread_;
    std::auto_ptr<boost::thread> uniqueThread_;
    mutable boost::mutex common_mutex_;
    mutable boost::mutex unique_mutex_;
    boost::condition common_condition_;
    boost::condition unique_condition_;
};

}

#endif
