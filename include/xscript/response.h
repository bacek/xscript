#ifndef _XSCRIPT_RESPONSE_H_
#define _XSCRIPT_RESPONSE_H_

#include <memory>
#include <string>
#include <boost/utility.hpp>

#include "xscript/types.h"

namespace xscript {

class BinaryWriter;
class Cookie;
class Request;

class Response : private boost::noncopyable {
public:
    Response();
    virtual ~Response();

    void setCookie(const Cookie &cookie);
    void setStatus(unsigned short status);
    void sendError(unsigned short status, const std::string &message);
    void setHeader(const std::string &name, const std::string &value);

    std::streamsize write(const char *buf, std::streamsize size, Request *request);
    std::streamsize write(std::auto_ptr<BinaryWriter> buf);
    std::string outputHeader(const std::string &name) const;

    void sendHeaders();

    void redirectBack(const Request *req);
    void redirectToPath(const std::string &path);

    void setContentType(const std::string &type);
    void setContentEncoding(const std::string &encoding);

    bool isBinary() const;
    unsigned short status() const; 
    const HeaderMap& outHeaders() const;
    const CookieSet& outCookies() const;
    
    void detach();
    
protected:
    virtual bool suppressBody(const Request *req) const;
    virtual void writeBuffer(const char *buf, std::streamsize size);
    virtual void writeByWriter(const BinaryWriter *writer);
    virtual void writeError(unsigned short status, const std::string &message);
    virtual void writeHeaders();
    
private:
    class ResponseData;
    friend class ResponseData;
    ResponseData *data_;
};

} // namespace xscript

#endif // _XSCRIPT_RESPONSE_H_
