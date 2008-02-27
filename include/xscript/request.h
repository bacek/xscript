#ifndef _XSCRIPT_REQUEST_H_
#define _XSCRIPT_REQUEST_H_

#include <string>
#include <vector>
#include <boost/utility.hpp>
#include <libxml/tree.h>

namespace xscript
{

class Request : private boost::noncopyable
{
public:
	Request();
	virtual ~Request();
	
	virtual unsigned short getServerPort() const = 0;
	virtual const std::string& getServerAddr() const = 0;

	virtual const std::string& getPathInfo() const = 0;
	virtual const std::string& getPathTranslated() const = 0;
	
	virtual const std::string& getScriptName() const = 0;
	virtual const std::string& getScriptFilename() const = 0;
	
	virtual const std::string& getDocumentRoot() const = 0;
	
	virtual const std::string& getRemoteUser() const = 0;
	virtual const std::string& getRemoteAddr() const = 0;
	virtual const std::string& getQueryString() const = 0;
	virtual const std::string& getRequestMethod() const = 0;
	
	virtual std::streamsize getContentLength() const = 0;
	virtual const std::string& getContentType() const = 0;
	
	virtual unsigned int countArgs() const = 0;
	virtual bool hasArg(const std::string &name) const = 0;
	virtual const std::string& getArg(const std::string &name) const = 0;
	virtual void getArg(const std::string &name, std::vector<std::string> &v) const = 0;
	virtual void argNames(std::vector<std::string> &v) const = 0;
	
	virtual unsigned int countHeaders() const = 0;
	virtual bool hasHeader(const std::string &name) const = 0;
	virtual const std::string& getHeader(const std::string &name) const = 0;
	virtual void headerNames(std::vector<std::string> &v) const = 0;
	virtual void addInputHeader(const std::string &name, const std::string &value) = 0;
	
	virtual unsigned int countCookies() const = 0;
	virtual bool hasCookie(const std::string &name) const = 0;
	virtual const std::string& getCookie(const std::string &name) const = 0;
	virtual void cookieNames(std::vector<std::string> &v) const = 0;

	virtual unsigned int countVariables() const = 0;
	virtual bool hasVariable(const std::string &name) const = 0;
	virtual const std::string& getVariable(const std::string &name) const = 0;
	virtual void variableNames(std::vector<std::string> &v) const = 0;

	virtual bool hasFile(const std::string &name) const = 0;
	virtual const std::string& remoteFileName(const std::string &name) const = 0;
	virtual const std::string& remoteFileType(const std::string &name) const = 0;
	virtual std::pair<const char*, std::streamsize> remoteFile(const std::string &name) const = 0;

	virtual bool isSecure() const = 0;
	virtual std::pair<const char*, std::streamsize> requestBody() const = 0;
	virtual bool suppressBody() const = 0;
};

} // namespace xscript

#endif // _XSCRIPT_REQUEST_H_
