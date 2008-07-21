#ifndef _XSCRIPT_REQUEST_IMPL_H_
#define _XSCRIPT_REQUEST_IMPL_H_

#include <set>
#include <map>
#include <iosfwd>
#include <functional>
#include <boost/cstdint.hpp>

#if defined(HAVE_STLPORT_HASHMAP)
#include <hash_set>
#include <hash_map>
#elif defined(HAVE_EXT_HASH_MAP) || defined(HAVE_GNUCXX_HASHMAP)
#include <ext/hash_set>
#include <ext/hash_map>
#endif

#include "xscript/component.h"
#include "xscript/functors.h"
#include "xscript/util.h"
#include "xscript/range.h"
#include "xscript/cookie.h"
#include "xscript/request.h"

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

class RequestImpl : public Request
{
public:
	RequestImpl();
	virtual ~RequestImpl();

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
	virtual bool suppressBody() const;
	
	virtual void reset();
	void attach(std::istream *is, char *env[]);

private:
	RequestImpl(const RequestImpl &);
	RequestImpl& operator = (const RequestImpl &);
	friend class Parser;

private:
	VarMap vars_, cookies_;
	std::vector<char> body_;
	HeaderMap headers_;

	std::map<std::string, File> files_;
	std::vector<StringUtils::NamedValue> args_;
};

class RequestFactory : public Component<RequestFactory>
{
public:
	RequestFactory();
	virtual ~RequestFactory();

	virtual std::auto_ptr<RequestImpl> create();
};

} // namespace xscript

#endif // _XSCRIPT_REQUEST_IMPL_H_
