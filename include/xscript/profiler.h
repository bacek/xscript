#ifndef _XSCRIPT_PROFILER_H_
#define _XSCRIPT_PROFILER_H_

#include <sys/time.h>
#include <boost/function.hpp>

namespace xscript
{

	template<typename Ret>
	std::pair<Ret, uint64_t> profile(const boost::function<Ret ()>& f) {
		struct timeval startTime, endTime;
		gettimeofday(&startTime, 0);

		Ret res = f();

		gettimeofday(&endTime, 0);
		uint64_t delta = (endTime.tv_sec - startTime.tv_sec) * 1000000 + endTime.tv_usec - startTime.tv_usec;

		return std::make_pair(res, delta);
	}

	uint64_t profile(const boost::function<void ()>& f) {
		struct timeval startTime, endTime;
		gettimeofday(&startTime, 0);

		f();

		gettimeofday(&endTime, 0);
		uint64_t delta = (endTime.tv_sec - startTime.tv_sec) * 1000000 + endTime.tv_usec - startTime.tv_usec;

		return delta;
	}

}


#endif
