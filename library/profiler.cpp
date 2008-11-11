#include "settings.h"

#include "xscript/profiler.h"
#include "xscript/logger.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

Profiler::~Profiler() {
    timeval endTime;
    gettimeofday(&endTime, 0);

    uint64_t delta = endTime - startTime_;

    log_->info("[profile] %llu.%06llu %s", (unsigned long long)(delta / 1000000),
	(unsigned long long)(delta % 1000000), info_.c_str());
}

} // namespace xscript
