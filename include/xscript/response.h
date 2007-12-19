#ifndef _XSCRIPT_RESPONSE_H_
#define _XSCRIPT_RESPONSE_H_

#include <string>
#include <boost/utility.hpp>

namespace xscript
{

class Cookie;
class Request;

class Response : private boost::noncopyable
{
public:
	Response();
	virtual ~Response();
	
	virtual void setCookie(const Cookie &cookie) = 0;
	virtual void setStatus(unsigned short status) = 0;
	virtual void sendError(unsigned short status) = 0;
	virtual void setHeader(const std::string &name, const std::string &value) = 0;

	virtual std::streamsize write(const char *buf, std::streamsize size) = 0;
	virtual std::string outputHeader(const std::string &name) const = 0;
	
	void redirectBack(const Request *req);
	void redirectToPath(const std::string &path);
	
	void setContentType(const std::string &type);
	void setContentEncoding(const std::string &encoding);
};

} // namespace xscript

#endif // _XSCRIPT_RESPONSE_H_
