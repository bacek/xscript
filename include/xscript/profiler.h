#ifndef _XSCRIPT_PROFILER_H_
#define _XSCRIPT_PROFILER_H_

#include <sys/time.h>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>

namespace xscript
{
    class Logger;

    /**
     * Calculate timeval delta in milliseconds
     */
    inline uint64_t operator-(const timeval& endTime, const timeval &startTime) {
		return (endTime.tv_sec - startTime.tv_sec) * (uint64_t)1000000 
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
	inline uint64_t profile(const boost::function<void ()>& f) {
		struct timeval startTime, endTime;
		gettimeofday(&startTime, 0);

		f();

		gettimeofday(&endTime, 0);

		return endTime - startTime;
	}

    /**
     * Scoped profiler.
     */
    class Profiler {
    public:
        /**
         * Create profiler.
         * \param log   Logger to output to.
         * \param info  Info string appened to output.
         */
        Profiler(Logger* log, const std::string& info)
            : log_(log), info_(info)
        {
            gettimeofday(&startTime_, 0);
        }

        /**
         * Destructor which output profiling info into log.
         */
        ~Profiler();
    private:
        Logger          *log_;
        std::string     info_;
        timeval         startTime_;
    };
}


#endif
