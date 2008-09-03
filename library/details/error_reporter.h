#ifndef _XSCRIPT_DETAILS_ERROR_REPORTER_H_
#define _XSCRIPT_DETAILS_ERROR_REPORTER_H_

#include <cstring>
#include <iostream>
#include <boost/type_traits.hpp>

namespace xscript {

template <typename Res>
class ErrorReporterImpl {
public:
	static void report(const char *what, int error, std::ostream &os);
};

template <typename Func>
class ErrorReporter {
public:
	static void report(const char *what, int error, std::ostream &os);
private:
	typedef boost::function_traits<typename boost::remove_pointer<Func>::type> FuncTraits;
};

template <typename Res> inline void
ErrorReporterImpl<Res>::report(const char *what, int error, std::ostream &os) {
    os << what << error;
}

template <> inline void
ErrorReporterImpl<char*>::report(const char *what, int error, std::ostream &os) {
	char buf[256];
	os << what << strerror_r(error, buf, sizeof(buf));
}

template <> inline void
ErrorReporterImpl<int>::report(const char *what, int error, std::ostream &os) {
	char buf[256];
	strerror_r(error, buf, sizeof(buf));
	os << what << buf;
}

template <typename Func> inline void
ErrorReporter<Func>::report(const char *what, int error, std::ostream &os) {
	ErrorReporterImpl<typename FuncTraits::result_type>::report(what, error, os);
}

template <typename Func> inline ErrorReporter<Func>
makeErrorReporter(Func) {
	return ErrorReporter<Func>();
}

} // namespace xscript

#endif // _XSCRIPT_DETAILS_ERROR_REPORTER_H_
