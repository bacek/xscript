#ifndef _XSCRIPT_INTERNAL_PARSER_H_
#define _XSCRIPT_INTERNAL_PARSER_H_

#include <string>

#include <boost/noncopyable.hpp>

#include "xscript/functors.h"
#include "xscript/range.h"
#include "xscript/request.h"
#include "xscript/string_utils.h"

namespace xscript {

class Encoder;

class Parser : private boost::noncopyable {
public:
    static std::string getBoundary(const Range &range);

    static void addCookie(Request::RequestImpl *req, const Range &range, Encoder *encoder);
    static void addHeader(Request::RequestImpl *req, const Range &key, const Range &value, Encoder *encoder);

    static void parse(Request::RequestImpl *req, char *env[], Encoder *encoder);
    static void parseCookies(Request::RequestImpl *req, const Range &range, Encoder *encoder);

    static void parsePart(Request::RequestImpl *req, Range &part, Encoder *encoder);
    static void parseLine(Range &line, std::map<Range, Range, RangeCILess> &m);
    static void parseMultipart(Request::RequestImpl *req, Range &data, const std::string &boundary, Encoder *encoder);

    template<typename Map> static bool has(const Map &m, const std::string &key);
    template<typename Map> static void keys(const Map &m, std::vector<std::string> &v);
    template<typename Map> static const std::string& get(const Map &m, const std::string &key);

    static const Range RN_RANGE;
    static const Range NAME_RANGE;
    static const Range FILENAME_RANGE;

    static const Range HEADER_RANGE;
    static const Range COOKIE_RANGE;
    static const Range EMPTY_LINE_RANGE;
    static const Range CONTENT_TYPE_RANGE;
    static const Range CONTENT_TYPE_MULTIPART_RANGE;
};

template<typename Map> inline bool
Parser::has(const Map &m, const std::string &key) {
    return m.find(key) != m.end();
}

template<typename Map> inline void
Parser::keys(const Map &m, std::vector<std::string> &v) {

    std::vector<std::string> tmp;
    tmp.reserve(m.size());

    for (typename Map::const_iterator i = m.begin(), end = m.end(); i != end; ++i) {
        tmp.push_back(i->first);
    }
    v.swap(tmp);
}

template<typename Map> inline const std::string&
Parser::get(const Map &m, const std::string &key) {
    typename Map::const_iterator i = m.find(key);
    return (m.end() == i) ? StringUtils::EMPTY_STRING : i->second;
}

} // namespace xscript

#endif // _XSCRIPT_INTERNAL_PARSER_H_
