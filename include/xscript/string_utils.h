#ifndef _XSCRIPT_STRING_UTILS_H_
#define _XSCRIPT_STRING_UTILS_H_

#include <string>
#include <ostream>

#include <xscript/range.h>

/**
 * Various string and url related utilities.
 */

namespace xscript {

class Encoder;

namespace StringUtils {
    const std::string EMPTY_STRING;
    typedef std::pair<std::string, std::string> NamedValue;

    void report(const char *what, int error, std::ostream &stream);

    std::string urlencode(const Range &val);
    template<typename Cont> static std::string urlencode(const Cont &cont) {
        return urlencode(createRange(cont));
    };

    std::string urldecode(const Range &val);
    template<typename Cont> static std::string urldecode(const Cont &cont) {
        return urldecode(createRange(cont));
    }

    void parse(const Range &range, std::vector<NamedValue> &v, Encoder *encoder = NULL);
    template<typename Cont> static void parse(const Cont &cont, std::vector<NamedValue> &v, Encoder *encoder = NULL) {
        parse(createRange(cont), v, encoder);
    }

    std::string tolower(const std::string& str);
    std::string toupper(const std::string& str);
    const char* nextUTF8(const char* data);
};


};

#endif // _XSCRIPT_STRING_UTILS_H_
