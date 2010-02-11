#ifndef _XSCRIPT_RESPONSE_H_
#define _XSCRIPT_RESPONSE_H_

#include <memory>
#include <string>

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include "xscript/types.h"

namespace xscript {

class BinaryWriter;
class Context;
class Cookie;
class PageCacheData;
class Request;

class Response : private boost::noncopyable {
public:
    Response();
    Response(std::ostream *stream);    
    virtual ~Response();

    void setCookie(const Cookie &cookie);
    void setStatus(unsigned short status);
    void sendError(unsigned short status, const std::string &message);
    void setHeader(const std::string &name, const std::string &value);
    void setExpireDelta(boost::uint32_t delta);
    void setSuppressBody(bool value);

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
    boost::uint32_t expireDelta() const;
    
    void detach(const Context *ctx);
    void setCacheable(
        boost::shared_ptr<PageCacheData> cache_data = boost::shared_ptr<PageCacheData>());
    
    virtual bool suppressBody(const Request *req) const;
    
protected:
    virtual void writeBuffer(const char *buf, std::streamsize size);
    virtual void writeByWriter(const BinaryWriter *writer);
    virtual void writeError(unsigned short status, const std::string &message);
    virtual void writeHeaders();
    
private:
    class ResponseData;
    friend class ResponseData;
    ResponseData *data_;
};

class ResponseDetacher {
public:
    explicit ResponseDetacher(Response *resp, const boost::shared_ptr<Context> &ctx);
    ~ResponseDetacher();

private:
    Response *resp_;
    const boost::shared_ptr<Context>& ctx_;
};

} // namespace xscript

#endif // _XSCRIPT_RESPONSE_H_
