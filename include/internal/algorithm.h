#ifndef _XSCRIPT_INTERNAL_ALGORITHM_H_
#define _XSCRIPT_INTERNAL_ALGORITHM_H_

#include <cstring>

#include "xscript/range.h"
#include "xscript/algorithm.h"

namespace xscript {

#ifdef HAVE_MEMCMP

inline bool
endsWith(const Range &range, const Range &substr) {
    if (substr.size() <= range.size()) {
        const char *i = range.begin() + range.size() - substr.size();
        return memcmp(i, substr.begin(), substr.size()) == 0;
    }
    return false;
}

inline bool
startsWith(const Range &range, const Range &substr) {
    if (substr.size() <= range.size()) {
        return memcmp(range.begin(), substr.begin(), substr.size()) == 0;
    }
    return false;
}

#endif // HAVE_MEMCMP

#ifdef HAVE_MEMCHR

inline bool
split(const Range &range, char c, Range &first, Range &second) {
    const char *i = (const char*) memchr(range.begin(), c, range.size());
    return doSplit(range, (NULL == i) ? range.end() : i, 1, first, second);
}

#endif // HAVE_MEMCHR

#ifdef HAVE_MEMMEM

inline bool
split(const Range &range, const Range &delim, Range &first, Range &second) {
    const char * i = (const char*) memmem(range.begin(), range.size(), delim.begin(), delim.size());
    return doSplit(range, (NULL == i) ? range.end() : i, delim.size(), first, second);
}

#endif // HAVE_MEMMEM

} // namespace xscript

#endif // _XSCRIPT_INTERNAL_ALGORITHM_H_
