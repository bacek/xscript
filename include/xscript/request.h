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
#include "xscript/types.h"
#include "xscript/request_file.h"

namespace xscript {

typedef std::vector<RequestFile> RequestFiles;

class RequestImpl;

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
    const std::string& getRequestURI() const;
    std::string getOriginalURI() const;
    const std::string& getHost() const;
    const std::string& getOriginalHost() const;
    std::string getOriginalUrl() const;

    boost::uint32_t getContentLength() const;
    const std::string& getContentType() const;
    const std::string& getContentEncoding() const;

    const std::string& getOriginalXForwardedFor() const;
    std::string getXForwardedFor() const;

    unsigned int countArgs() const;
    bool hasArgData(const std::string &name) const;
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
    HeaderMap headers() const;

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
    const RequestFiles* getFiles(const std::string &name) const;

    bool isSecure() const;
    bool isBot() const;
    std::pair<const char*, std::streamsize> requestBody() const;
    bool suppressBody() const;

    bool hasPostData() const;

    boost::uint64_t requestID() const; // 0 - for children

    void attach(std::istream *is, char *env[]);

    static const std::string X_FORWARDED_FOR_HEADER_NAME;

    class HandlerRegisterer;
protected:
    static const std::string ATTACH_METHOD;
    static const std::string REAL_IP_METHOD;
    static const std::string ORIGINAL_URI_METHOD;
    static const std::string ORIGINAL_HOST_METHOD;
    
private:
    friend class Parser;
    friend class RequestImpl;
    
    class AttachHandler;
    class RealIPHandler;
    class OriginalURIHandler;
    class OriginalHostHandler;
    
    std::auto_ptr<RequestImpl> data_;
};

} // namespace xscript

#endif // _XSCRIPT_REQUEST_H_
