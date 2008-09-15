#ifndef _XSCRIPT_DETAILS_DEFAULT_REQUEST_RESPONSE_H
#define _XSCRIPT_DETAILS_DEFAULT_REQUEST_RESPONSE_H

#include <map>
#include <vector>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <boost/thread/mutex.hpp>

#include "xscript/functors.h"
#include "xscript/request_impl.h"
#include "xscript/request_response.h"
#include "xscript/util.h"

namespace xscript {

class DefaultRequestResponse : public RequestResponse {
public:
    DefaultRequestResponse();
    virtual ~DefaultRequestResponse();

    virtual unsigned short getServerPort() const;
    virtual const std::string& getServerAddr() const;

    virtual const std::string& getPathInfo() const;
    virtual const std::string& getPathTranslated() const;

    virtual const std::string& getScriptName() const;
    virtual const std::string& getScriptFilename() const;
    virtual const std::string& getDocumentRoot() const;

    virtual const std::string& getRemoteUser() const;
    virtual const std::string& getRemoteAddr() const;
    virtual const std::string& getRealIP() const;
    virtual const std::string& getQueryString() const;
    virtual const std::string& getRequestMethod() const;
    virtual std::string getURI() const;
    virtual std::string getOriginalURI() const;
    virtual std::string getHost() const;
    virtual std::string getOriginalHost() const;
    virtual std::string getOriginalUrl() const;

    virtual std::streamsize getContentLength() const;
    virtual const std::string& getContentType() const;
    virtual const std::string& getContentEncoding() const;

    virtual unsigned int countArgs() const;
    virtual bool hasArg(const std::string &name) const;
    virtual const std::string& getArg(const std::string &name) const;
    virtual void getArg(const std::string &name, std::vector<std::string> &v) const;
    virtual void argNames(std::vector<std::string> &v) const;
    virtual void setArg(const std::string &name, const std::string &value);

    virtual unsigned int countHeaders() const;
    virtual bool hasHeader(const std::string &name) const;
    virtual const std::string& getHeader(const std::string &name) const;
    virtual void headerNames(std::vector<std::string> &v) const;
    virtual void addInputHeader(const std::string &name, const std::string &value);

    virtual unsigned int countCookies() const;
    virtual bool hasCookie(const std::string &name) const;
    virtual const std::string& getCookie(const std::string &name) const;
    virtual void cookieNames(std::vector<std::string> &v) const;
    virtual void addInputCookie(const std::string &name, const std::string &value);

    virtual unsigned int countVariables() const;
    virtual bool hasVariable(const std::string &name) const;
    virtual const std::string& getVariable(const std::string &name) const;
    virtual void variableNames(std::vector<std::string> &v) const;
    virtual void setVariable(const std::string &name, const std::string &value);

    virtual bool hasFile(const std::string &name) const;
    virtual const std::string& remoteFileName(const std::string &name) const;
    virtual const std::string& remoteFileType(const std::string &name) const;
    virtual std::pair<const char*, std::streamsize> remoteFile(const std::string &name) const;

    virtual bool isSecure() const;
    virtual std::pair<const char*, std::streamsize> requestBody() const;

    virtual void setCookie(const Cookie &cookie);
    virtual void setStatus(unsigned short status);

    virtual void setHeader(const std::string &name, const std::string &value);
    virtual std::streamsize write(const char *buf, std::streamsize size);
    virtual std::string outputHeader(const std::string &name) const;

    virtual void sendError(unsigned short status, const std::string& message);
    virtual void sendHeaders();
    virtual bool suppressBody() const;

    void attach(std::istream *is, char *env[]);

private:
    DefaultRequestResponse(const DefaultRequestResponse &);
    DefaultRequestResponse& operator = (const DefaultRequestResponse &);

private:
    std::auto_ptr<RequestImpl> impl_;
    mutable boost::mutex mutex_;
};

} // namespace xscript

#endif // _XSCRIPT_DETAILS_DEFAULT_REQUEST_RESPONSE_H


