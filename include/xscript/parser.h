#ifndef _XSCRIPT_HELPER_H_
#define _XSCRIPT_HELPER_H_

#include <map>
#include <string>
#include <vector>
#include <iosfwd>
#include <boost/utility.hpp>

#include "settings.h"
#include "internal/functors.h"
#include "xscript/util.h"
#include "xscript/range.h"

namespace xscript
{

class File;
class Encoder;
class RequestImpl;

class Parser : private boost::noncopyable
{
public:
	
	static const char* statusToString(short status);
	static std::string getBoundary(const Range &range);
	
	static void addCookie(RequestImpl *req, const Range &range, Encoder *encoder);
	static void addHeader(RequestImpl *req, const Range &key, const Range &value, Encoder *encoder);

	static void parse(RequestImpl *req, char *env[], Encoder *encoder);
	static void parseCookies(RequestImpl *req, const Range &range, Encoder *encoder);
	
	static void parsePart(RequestImpl *req, Range &part, Encoder *encoder);
	static void parseLine(Range &line, std::map<Range, Range, RangeCILess> &m);
	static void parseMultipart(RequestImpl *req, Range &data, const std::string &boundary, Encoder *encoder);

	template<typename Map> static bool has(const Map &m, const std::string &key);
	template<typename Map> static void keys(const Map &m, std::vector<std::string> &v);
	template<typename Map> static const std::string& get(const Map &m, const std::string &key);

	static std::string normalizeInputHeaderName(const Range &range);
	static std::string normalizeOutputHeaderName(const std::string &name);
	
	static const Range RN_RANGE;
	static const Range NAME_RANGE;
	static const Range FILENAME_RANGE;
	
	static const Range HEADER_RANGE;
	static const Range COOKIE_RANGE;
	static const Range EMPTY_LINE_RANGE;
	static const Range CONTENT_TYPE_RANGE;
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

#endif // _XSCRIPT_HELPER_H_
