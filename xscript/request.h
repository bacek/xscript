#ifndef _XSCRIPT_STANDALONE_REQUEST_H_
#define _XSCRIPT_STANDALONE_REQUEST_H_

#include <map>
#include <vector>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#if defined(HAVE_STLPORT_HASHMAP)
#include <hash_map>
#elif defined(HAVE_EXT_HASH_MAP) || defined(HAVE_GNUCXX_HASHMAP)
#include <ext/hash_map>
#endif

#include "xscript/util.h"
#include "xscript/request.h"
#include "xscript/response.h"

namespace xscript
{

class StandaloneRequest : public Request, public Response
{
public:
	StandaloneRequest(const std::string &url);
	virtual ~StandaloneRequest();
	
	virtual unsigned short getServerPort() const;
	virtual const std::string& getServerAddr() const;

	virtual const std::string& getPathInfo() const;
	virtual const std::string& getPathTranslated() const;
	
	virtual const std::string& getScriptName() const;
	virtual const std::string& getScriptFilename() const;
	virtual const std::string& getDocumentRoot() const;
	
	virtual const std::string& getRemoteUser() const;
	virtual const std::string& getRemoteAddr() const;
	virtual const std::string& getQueryString() const;
	virtual const std::string& getRequestMethod() const;

	virtual std::streamsize getContentLength() const;
	virtual const std::string& getContentType() const;
	
	virtual unsigned int countArgs() const;
	virtual bool hasArg(const std::string &name) const;
	virtual const std::string& getArg(const std::string &name) const;
	virtual void getArg(const std::string &name, std::vector<std::string> &v) const;
	virtual void argNames(std::vector<std::string> &v) const;
	
	virtual unsigned int countHeaders() const;
	virtual bool hasHeader(const std::string &name) const;
	virtual const std::string& getHeader(const std::string &name) const;
	virtual void headerNames(std::vector<std::string> &v) const;
	
	virtual unsigned int countCookies() const;
	virtual bool hasCookie(const std::string &name) const;
	virtual const std::string& getCookie(const std::string &name) const;
	virtual void cookieNames(std::vector<std::string> &v) const;

	virtual unsigned int countVariables() const;
	virtual bool hasVariable(const std::string &name) const;
	virtual const std::string& getVariable(const std::string &name) const;
	virtual void variableNames(std::vector<std::string> &v) const;

	virtual bool hasFile(const std::string &name) const;
	virtual const std::string& remoteFileName(const std::string &name) const;
	virtual const std::string& remoteFileType(const std::string &name) const;
	virtual std::pair<const char*, std::streamsize> remoteFile(const std::string &name) const;

	virtual bool isSecure() const;
	virtual std::pair<const char*, std::streamsize> requestBody() const;
	
	virtual void clear();
	
	virtual void setCookie(const Cookie &cookie);
	virtual void setStatus(unsigned short status);
	
	virtual void setHeader(const std::string &name, const std::string &value);
	virtual std::streamsize write(const char *buf, std::streamsize size);
	virtual std::string outputHeader(const std::string &name) const;

	void addHeader(const std::pair<std::string, std::string> &header);

private:
	StandaloneRequest(const StandaloneRequest &);
	StandaloneRequest& operator = (const StandaloneRequest &);

private:
	std::map<std::string, std::string> headers_;
	std::vector<StringUtils::NamedValue> params_;
};

} // namespace xscript

#endif // _XSCRIPT_STANDALONE_REQUEST_H_
