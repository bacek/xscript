#ifndef _XSCRIPT_UTIL_H_
#define _XSCRIPT_UTIL_H_

#include <sys/time.h>
#include <ctime>
#include <map>
#include <string>
#include <vector>
#include <iosfwd>
#include <stdexcept>
#include <boost/cstdint.hpp>
#include <boost/utility.hpp>
#include <xscript/config.h>
#include <xscript/range.h>
#include <xscript/xml_helpers.h>

#include <xscript/logger.h>

namespace xscript {

class UnboundRuntimeError : public std::exception {
public:
    UnboundRuntimeError(const std::string& error) : error_(error) {}
    virtual ~UnboundRuntimeError() throw () {}
    virtual const char* what() const throw () {
        return error_.c_str();
    }

private:
    std::string error_;
};

class XmlNodeRuntimeError : public std::runtime_error {
public:
    XmlNodeRuntimeError(const std::string& error) : std::runtime_error(error) {}
    XmlNodeRuntimeError(const std::string& error, XmlNodeHelper node) : std::runtime_error(error), node_(node) {}
    virtual ~XmlNodeRuntimeError() throw () {}
    virtual xmlNodePtr what_node() const throw() {
        return node_.get();
    }

private:
    XmlNodeHelper node_;
};

class InvokeError : public UnboundRuntimeError {
public:
    typedef std::vector<std::pair<std::string, std::string> > InfoMapType;

    InvokeError(const std::string &error) : UnboundRuntimeError(error) {}
    InvokeError(const std::string &error, const std::string &name, const std::string &value);

    void add(const std::string &name, const std::string &value);
    void addEscaped(const std::string &name, const std::string &value);

    virtual ~InvokeError() throw () {}
    virtual std::string what_info() const throw();
    virtual const InfoMapType& info() const throw() {
        return info_;
    }
private:
    InfoMapType info_;
};

class CriticalInvokeError : public InvokeError {
public:
    CriticalInvokeError(const std::string &error) : InvokeError(error) {}
    CriticalInvokeError(const std::string &error, const std::string &name, const std::string &value) :
        InvokeError(error, name, value) {}
};

class SkipResultInvokeError : public InvokeError {
public:
    SkipResultInvokeError(const std::string &error) : InvokeError(error) {}
    SkipResultInvokeError(const std::string &error, const std::string &name, const std::string &value) :
        InvokeError(error, name, value) {}
};


class HttpDateUtils : private boost::noncopyable {
public:
    static time_t parse(const char *value);
    static std::string format(time_t value);

private:
    HttpDateUtils();
    virtual ~HttpDateUtils();
};

class HashUtils : private boost::noncopyable {
public:
    typedef std::vector<char> ByteArrayType;

    static std::string hexMD5(const char *key);
    static std::string hexMD5(const ByteArrayType &key);
    static std::string hexMD5(const char *key, unsigned long len);
    static std::string blowfish(const char *data, const char *key, const char *ivec);
    static std::string blowfish(const ByteArrayType &data, const ByteArrayType &key, const char *ivec);
    static std::string blowfish(const char *data, unsigned long data_len,
                                const char *key, unsigned long key_len, const char *ivec);
    static boost::uint32_t crc32(const std::string &key);
    static boost::uint32_t crc32(const char *key, unsigned long len);
private:
    HashUtils();
    virtual ~HashUtils();
};

class FileUtils : private boost::noncopyable {
public:
    static std::string normalize(const std::string &filepath);
    static bool fileExists(const std::string &name);
    static time_t modified(const std::string &name);

private:
    FileUtils();
    virtual ~FileUtils();
};


class TimeoutCounter {
public:
    TimeoutCounter();
    TimeoutCounter(int timeout);
    ~TimeoutCounter();

    void reset(int timeout);
    bool unlimited() const;
    bool expired() const;
    int remained() const;
    int elapsed() const;
    void timeout(int timeout);
    int timeout() const;

    static const int UNLIMITED_TIME;
private:
    struct timeval init_time_;
    int timeout_;
};

void terminate(int status, const char* message, bool write_log);

} // namespace xscript

#endif // _XSCRIPT_UTIL_H_
