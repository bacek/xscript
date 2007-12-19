#ifndef _XSCRIPT_RANGE_H_
#define _XSCRIPT_RANGE_H_

#include <string>
#include <vector>
#include <algorithm>
#include <boost/range.hpp>

namespace xscript
{

typedef boost::iterator_range<const char*> Range;

inline Range
createRange(const char *str) {
	return Range(str, str + std::char_traits<char>::length(str));
}

inline Range
createRange(const std::string &str) {
	return Range(str.c_str(), str.c_str() + str.size());
}

inline Range
createRange(const std::vector<char> &vec) {
	return vec.empty() ? Range() : Range(&vec[0], &vec[0] + vec.size());
}

} // namespace xscript

#endif // _XSCRIPT_RANGE_H_
