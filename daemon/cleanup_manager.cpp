#include "settings.h"

#include <boost/bind.hpp>

#include "cleanup_manager.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

CleanupManager::CleanupManager() : stopped_(false), max_size_(0) {}

CleanupManager::CleanupManager(unsigned int max_size) : stopped_(false)
{
    init(max_size);
}

CleanupManager::~CleanupManager() {
    if (commonThread_.get()) {
        commonThread_->join();
    }
    if (uniqueThread_.get()) {
        uniqueThread_->join();
    }
}

void
CleanupManager::init(unsigned int max_size) {
    max_size_ = max_size;
    commonThread_ = std::auto_ptr<boost::thread>(
        new boost::thread(boost::bind(&CleanupManager::commonWorker, this)));
    uniqueThread_ = std::auto_ptr<boost::thread>(
        new boost::thread(boost::bind(&CleanupManager::uniqueWorker, this)));
}

void
CleanupManager::commonWorker() {
    while(true) {
        boost::mutex::scoped_lock lock(common_mutex_);
        common_condition_.wait(lock, boost::bind(&CleanupManager::commonRun, this));
        if (stopped_) {
            return;
        }
        boost::mutex::scoped_lock unique_lock(unique_mutex_);
        bool need_notify = false;
        int remained = max_size_ - unique_.size();
        for(ListType::iterator it = common_.begin();
            it != common_.end();
            ++it) {
            if (remained <= 0) {
                break;
            }
            if (it->unique()) {                
                unique_.push_back(*it);
                it = common_.erase(it);
                --remained;
                need_notify = true;
            }
        }
        unique_lock.unlock();
        if (need_notify) {
            unique_condition_.notify_all();
        }
    }
}

void
CleanupManager::uniqueWorker() {
    while(true) {
        boost::mutex::scoped_lock lock(unique_mutex_);
        unique_condition_.wait(lock, boost::bind(&CleanupManager::uniqueRun, this));
        if (stopped_) {
            return;
        }
        unique_.pop_front();
        common_condition_.notify_all();
    }
}

void
CleanupManager::push(boost::shared_ptr<Context> ctx) {
    boost::mutex::scoped_lock lock(common_mutex_);
    if (common_.size() >= max_size_) {
        return;
    }
    common_.push_back(ctx);
    lock.unlock();
    common_condition_.notify_all();
}

bool
CleanupManager::uniqueFull() const {
    boost::mutex::scoped_lock lock(unique_mutex_);
    return unique_.size() >= max_size_;
}

bool
CleanupManager::commonRun() const {
    return (!common_.empty() && !uniqueFull()) || stopped_;
}

bool
CleanupManager::uniqueRun() const {
    return !unique_.empty() || stopped_;
}

}
