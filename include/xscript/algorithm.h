#ifndef _XSCRIPT_ALGORITHM_H_
#define _XSCRIPT_ALGORITHM_H_

#include <algorithm>
#include <cctype>
#include <string>

namespace xscript {

template <typename Range> inline bool
endsWith(const Range &range, const Range &substr) {
    if (substr.size() <= range.size()) {
        typename Range::const_iterator i = range.begin();
        std::advance(i, range.size() - substr.size());
        return std::equal(substr.begin(), substr.end(), i);
    }
    return false;
}

template <typename Range> inline bool
startsWith(const Range &range, const Range &substr) {
    if (substr.size() <= range.size()) {
        return std::equal(substr.begin(), substr.end(), range.begin());
    }
    return false;
}

template <typename Range> inline Range
trim(const Range &range) {
    typename Range::const_iterator begin = range.begin(), end = range.end();
    while (begin != end && isspace(*begin)) {
        ++begin;
    }
    while (begin != end && isspace(*(end - 1))) {
        --end;
    }
    return Range(begin, end);
}

template<typename Range> inline Range
truncate(Range &range, typename Range::size_type left, typename Range::size_type right) {
    typename Range::const_iterator begin = range.begin(), end = range.end();
    while (begin != end && left--) {
        ++begin;
    }
    while (begin != end && right--) {
        --end;
    }
    return Range(begin, end);
}

template <typename Range> inline bool
doSplit(const Range &range, typename Range::const_iterator pos,
        typename Range::size_type size, Range &first, Range &second) {
    typename Range::const_iterator begin = range.begin(), end = range.end();
    first = Range(begin, pos);
    if (pos != end) {
        second = Range(pos + size, end);
        return true;
    }
    else {
        second = Range(end, end);
        return false;
    }
}

template <typename Range> inline bool
split(const Range &range, char c, Range &first, Range &second) {
    return doSplit(range, std::find(range.begin(), range.end(), c), 1, first, second);
}

template <typename Range> inline bool
split(const Range &range, const Range &delim, Range &first, Range &second) {
    return doSplit(range, std::search(range.begin(), range.end(), delim.begin(), delim.end()), delim.size(), first, second);
}

template <typename Range> inline bool
splitFirstOf(const Range &range, const char *chars, Range &first, Range &second) {
    return doSplit(range, std::find_first_of(range.begin(), range.end(),
                   chars, chars + std::char_traits<char>::length(chars)), 1, first, second);
}

} // namespace xscript

#endif // _XSCRIPT_ALGORITHM_H_
