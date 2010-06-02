#include "settings.h"

#include "xscript/logger.h"
#include "xscript/profiler.h"
#include "xscript/string_utils.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

boost::uint64_t
operator - (const timeval &endTime, const timeval &startTime) {
    return (endTime.tv_sec - startTime.tv_sec) * 
      static_cast<boost::uint64_t>(1000000) + endTime.tv_usec - startTime.tv_usec;
};

boost::uint64_t
profile(const boost::function<void ()>& f) {
    struct timeval startTime, endTime;
    gettimeofday(&startTime, 0);

    f();

    gettimeofday(&endTime, 0);

    return endTime - startTime;
}

struct Profiler::ProfilerData {
    ProfilerData(Logger* log, const std::string& info) : active_(true), log_(log), info_(info) {}
    
    bool active_;
    Logger *log_;
    std::string info_;
    timeval startTime_;
};

Profiler::Profiler(Logger* log, const std::string& info) :
    data_(new ProfilerData(log, info))
{
    gettimeofday(&data_->startTime_, 0);
}

boost::uint64_t
Profiler::release() {
    boost::uint64_t delta = checkPoint(StringUtils::EMPTY_STRING);
    data_->active_ = false;
    return delta;
}

boost::uint64_t
Profiler::checkPoint(const std::string &message) {
    if (!data_->active_) {
        return 0;
    }
    
    timeval endTime;
    gettimeofday(&endTime, 0);

    uint64_t delta = endTime - data_->startTime_;

    data_->log_->info("[profile] %llu.%06llu %s",
                      (unsigned long long)(delta / 1000000),
                      (unsigned long long)(delta % 1000000),
                      message.empty() ? data_->info_.c_str() : message.c_str());
    
    return delta;
}


void
Profiler::setInfo(const std::string &info) {
    data_->info_ = info;
}

Profiler::~Profiler() {
    release();
}

} // namespace xscript
