#ifndef _XSCRIPT_REQUEST_H_
#define _XSCRIPT_REQUEST_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/cstdint.hpp>
#include <boost/utility.hpp>

#include "xscript/functors.h"
#include "xscript/string_utils.h"

namespace xscript {

class Request : private boost::noncopyable {
public:
    Request();
    virtual ~Request();

    unsigned short getServerPort() const;
    const std::string& getServerAddr() const;

    const std::string& getPathInfo() const;
    const std::string& getPathTranslated() const;

    const std::string& getScriptName() const;
    const std::string& getScriptFilename() const;

    const std::string& getDocumentRoot() const;

    const std::string& getRemoteUser() const;
    const std::string& getRemoteAddr() const;
    const std::string& getRealIP() const;
    const std::string& getQueryString() const;
    const std::string& getRequestMethod() const;
    std::string getURI() const;
    std::string getOriginalURI() const;
    const std::string& getHost() const;
    const std::string& getOriginalHost() const;
    std::string getOriginalUrl() const;

    boost::uint32_t getContentLength() const;
    const std::string& getContentType() const;
    const std::string& getContentEncoding() const;

    unsigned int countArgs() const;
    bool hasArg(const std::string &name) const;
    const std::string& getArg(const std::string &name) const;
    void getArg(const std::string &name, std::vector<std::string> &v) const;
    void argNames(std::vector<std::string> &v) const;
    const std::vector<StringUtils::NamedValue>& args() const;

    unsigned int countHeaders() const;
    bool hasHeader(const std::string &name) const;
    const std::string& getHeader(const std::string &name) const;
    void headerNames(std::vector<std::string> &v) const;
    void addInputHeader(const std::string &name, const std::string &value);

    unsigned int countCookies() const;
    bool hasCookie(const std::string &name) const;
    const std::string& getCookie(const std::string &name) const;
    void cookieNames(std::vector<std::string> &v) const;

    unsigned int countVariables() const;
    bool hasVariable(const std::string &name) const;
    const std::string& getVariable(const std::string &name) const;
    void variableNames(std::vector<std::string> &v) const;

    unsigned int countFiles() const;
    bool hasFile(const std::string &name) const;
    const std::string& remoteFileName(const std::string &name) const;
    const std::string& remoteFileType(const std::string &name) const;
    std::pair<const char*, std::streamsize> remoteFile(const std::string &name) const;
    void fileNames(std::vector<std::string> &v) const;

    bool isSecure() const;
    bool isBot() const;
    std::pair<const char*, std::streamsize> requestBody() const;
    bool suppressBody() const;
    
    void attach(std::istream *is, char *env[]);

    class HandlerRegisterer;
protected:
    static const std::string ATTACH_METHOD;
    static const std::string REAL_IP_METHOD;
    static const std::string ORIGINAL_URI_METHOD;
    static const std::string ORIGINAL_HOST_METHOD;
    
private:
    friend class Parser;
    
    class AttachHandler;
    friend class AttachHandler;
    class RealIPHandler;
    friend class RealIPHandler;
    class OriginalURIHandler;
    friend class OriginalURIHandler;
    class OriginalHostHandler;
    friend class OriginalHostHandler;
    
    class Request_Data;
    friend class Request_Data;
    Request_Data *data_;
};

class Parser : private boost::noncopyable {
public:
    static const char* statusToString(short status);
    static std::string getBoundary(const Range &range);

    static void addCookie(Request *req, const Range &range, Encoder *encoder);
    static void addHeader(Request *req, const Range &key, const Range &value, Encoder *encoder);

    static void parse(Request *req, char *env[], Encoder *encoder);
    static void parseCookies(Request *req, const Range &range, Encoder *encoder);

    static void parsePart(Request *req, Range &part, Encoder *encoder);
    static void parseLine(Range &line, std::map<Range, Range, RangeCILess> &m);
    static void parseMultipart(Request *req, Range &data, const std::string &boundary, Encoder *encoder);

    template<typename Map> static bool has(const Map &m, const std::string &key);
    template<typename Map> static void keys(const Map &m, std::vector<std::string> &v);
    template<typename Map> static const std::string& get(const Map &m, const std::string &key);

    static bool normalizeHeader(const std::string &name, const Range &value, std::string &result);
    static std::string checkUrlEscaping(const Range &range);
    static Range checkHost(const Range &range);
    
    static std::string normalizeInputHeaderName(const Range &range);
    static std::string normalizeOutputHeaderName(const std::string &name);
    static std::string normalizeQuery(const Range &range);

    static const Range RN_RANGE;
    static const Range NAME_RANGE;
    static const Range FILENAME_RANGE;

    static const Range HEADER_RANGE;
    static const Range COOKIE_RANGE;
    static const Range EMPTY_LINE_RANGE;
    static const Range CONTENT_TYPE_RANGE;
    static const Range CONTENT_TYPE_MULTIPART_RANGE;
    
    static const std::string NORMALIZE_HEADER_METHOD;
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

#endif // _XSCRIPT_REQUEST_H_
