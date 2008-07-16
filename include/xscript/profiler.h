#ifndef _XSCRIPT_PROFILER_H_
#define _XSCRIPT_PROFILER_H_

#include <sys/time.h>
#include <boost/function.hpp>

namespace xscript
{

    /**
     * Calculate timeval delta in milliseconds
     */
    uint64_t operator-(const timeval& endTime, const timeval &startTime) {
		return (endTime.tv_sec - startTime.tv_sec) * 1000000 
            + endTime.tv_usec - startTime.tv_usec;
    };

    /**
     * Profile some function.
     * Wraps function end returns pair of function res and passed time.
     */
	template<typename Ret>
	std::pair<Ret, uint64_t> profile(const boost::function<Ret ()>& f) {
		struct timeval startTime, endTime;
		gettimeofday(&startTime, 0);

		Ret res = f();

		gettimeofday(&endTime, 0);
		return std::make_pair(res, endTime - startTime);
	}

    /**
     * Specialisation of profile for functions returning void.
     * Return just passed time.
     */
	uint64_t profile(const boost::function<void ()>& f) {
		struct timeval startTime, endTime;
		gettimeofday(&startTime, 0);

		f();

		gettimeofday(&endTime, 0);

		return endTime - startTime;
	}

}


#endif
