#include "settings.h"

#include "xscript/profiler.h"
#include "xscript/logger.h"

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

Profiler::Profiler(Logger* log, const std::string& info) : log_(log), info_(info) {
    gettimeofday(&startTime_, 0);
}

Profiler::~Profiler() {
    timeval endTime;
    gettimeofday(&endTime, 0);

    uint64_t delta = endTime - startTime_;

    log_->info("[profile] %llu.%06llu %s", (unsigned long long)(delta / 1000000),
	(unsigned long long)(delta % 1000000), info_.c_str());
}

} // namespace xscript
