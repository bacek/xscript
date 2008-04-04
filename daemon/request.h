#ifndef _XSCRIPT_SERVER_REQUEST_H_
#define _XSCRIPT_SERVER_REQUEST_H_

#include <set>
#include <map>
#include <iosfwd>
#include <functional>
#include <boost/cstdint.hpp>
#include <boost/thread/mutex.hpp>

#include "settings.h"

#if defined(HAVE_STLPORT_HASHMAP)
#include <hash_set>
#include <hash_map>
#elif defined(HAVE_EXT_HASH_MAP) || defined(HAVE_GNUCXX_HASHMAP)
#include <ext/hash_set>
#include <ext/hash_map>
#endif

#include "internal/functors.h"
#include "xscript/util.h"
#include "xscript/range.h"
#include "xscript/cookie.h"
#include "xscript/request.h"
#include "xscript/response.h"

namespace xscript
{

class File
{
public:
	File(const std::map<Range, Range, RangeCILess> &m, const Range &content);
	
	const std::string& type() const;
	const std::string& remoteName() const;
	
	std::pair<const char*, std::streamsize> data() const;
	
private:
	std::string name_, type_;
	std::pair<const char*, std::streamsize> data_;
};

#if defined(HAVE_GNUCXX_HASHMAP)

typedef __gnu_cxx::hash_map<std::string, std::string, StringCIHash> VarMap;
typedef __gnu_cxx::hash_map<std::string, std::string, StringCIHash, StringCIEqual> HeaderMap;

#elif defined(HAVE_EXT_HASH_MAP) || defined(HAVE_STLPORT_HASHMAP)

typedef std::hash_map<std::string, std::string, StringCIHash> VarMap;
typedef std::hash_map<std::string, std::string, StringCIHash, StringCIEqual> HeaderMap;

#else

typedef std::map<std::string, std::string> VarMap;
typedef std::map<std::string, std::string, StringCILess> HeaderMap;

#endif

class Parser;

class ServerRequest : public Request, public Response
{
public:
	ServerRequest();
	virtual ~ServerRequest();

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
	
	virtual std::streamsize getContentLength() const;
	virtual const std::string& getContentType() const;
	virtual const std::string& getContentEncoding() const;
	
	virtual unsigned int countArgs() const;
	virtual bool hasArg(const std::string &name) const;
	virtual const std::string& getArg(const std::string &name) const;
	virtual void getArg(const std::string &name, std::vector<std::string> &v) const;
	virtual void argNames(std::vector<std::string> &v) const;
	
	virtual unsigned int countHeaders() const;
	virtual bool hasHeader(const std::string &name) const;
	virtual const std::string& getHeader(const std::string &name) const;
	virtual void headerNames(std::vector<std::string> &v) const;
	virtual void addInputHeader(const std::string &name, const std::string &value);
	
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
	
	virtual void setCookie(const Cookie &cookie);
	virtual void setStatus(unsigned short status);
	virtual void sendError(unsigned short status, const std::string& message);
	
	virtual void setHeader(const std::string &name, const std::string &value);
	virtual std::streamsize write(const char *buf, std::streamsize size);
	virtual std::string outputHeader(const std::string &name) const;

	bool suppressBody() const;

	void reset();
	void sendHeaders();
	void attach(std::istream *is, std::ostream *os, char *env[]);
	
private:
	ServerRequest(const ServerRequest &);
	ServerRequest& operator = (const ServerRequest &);
	void sendHeadersInternal();
	friend class Parser;

private:
	bool headers_sent_;
	unsigned short status_;
	
	std::ostream *stream_;
	mutable boost::mutex mutex_;
	
	VarMap vars_, cookies_;
	std::vector<char> body_;
	HeaderMap headers_, out_headers_;

	std::map<std::string, File> files_;
	std::set<Cookie, CookieLess> out_cookies_;
	std::vector<StringUtils::NamedValue> args_;
};

} // namespace xscript

#endif // _XSCRIPT_SERVER_REQUEST_H_
