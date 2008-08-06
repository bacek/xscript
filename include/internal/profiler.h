#ifndef _XSCRIPT_INTERNAL_PROFILER_H_
#define _XSCRIPT_INTERNAL_PROFILER_H_

#include "settings.h"

#include <stdexcept>

#include <iomanip>
#include <sstream>

#include <sys/time.h>
#include <sys/times.h>

namespace xscript {
namespace internal {

/**
 * TODO: Remove this class?
 */
class Profiler {
public:
    Profiler(const std::string& funcName) :
            funcName_(funcName) {
        gettimeofday(&startTime_, 0);
        times(&startCPUTime_);
    }

    std::string getInfo() {
        times(&finishCPUTime_);
        gettimeofday(&finishTime_, 0);
        struct timeval delta;
        delta.tv_sec = finishTime_.tv_sec - startTime_.tv_sec;
        delta.tv_usec = finishTime_.tv_usec - startTime_.tv_usec;
        std::stringstream os;
        os << "'" << funcName_ << "' time="
        << delta.tv_sec << "."
        << std::setw(6) << std::setfill('0') << delta.tv_usec << "; "
        << std::setfill(' ')
        << "user=" << std::setw(6)
        << ticks2sec(startCPUTime_.tms_utime, finishCPUTime_.tms_utime)
        << "; system=" << std::setw(6)
        << ticks2sec(startCPUTime_.tms_stime, finishCPUTime_.tms_stime);

        return os.str();
    }


    virtual ~Profiler() {
    }

protected:
    double ticks2sec(double from, double to) {
        return (to - from)/sysconf(_SC_CLK_TCK);
    }

private:
    const std::string funcName_;
    struct timeval startTime_;
    struct timeval finishTime_;
    struct tms startCPUTime_;
    struct tms finishCPUTime_;
};

} // namespace internal
} // namespace xscript

#endif //_XSCRIPT_INTERNAL_PROFILER_H_

