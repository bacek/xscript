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
    virtual const InfoMapType& what_info() const throw() {
        return info_;
    }
private:
    InfoMapType info_;
};

class Encoder;

class StringUtils : private boost::noncopyable {
public:
    static const std::string EMPTY_STRING;
    typedef std::pair<std::string, std::string> NamedValue;

    static void report(const char *what, int error, std::ostream &stream);

    static std::string urlencode(const Range &val);
    template<typename Cont> static std::string urlencode(const Cont &cont);

    static std::string urldecode(const Range &val);
    template<typename Cont> static std::string urldecode(const Cont &cont);

    static void parse(const Range &range, std::vector<NamedValue> &v, Encoder *encoder = NULL);
    template<typename Cont> static void parse(const Cont &cont, std::vector<NamedValue> &v, Encoder *encoder = NULL);

    static std::string tolower(const std::string& str);
    static std::string toupper(const std::string& str);
    static const char* nextUTF8(const char* data);

private:
    StringUtils();
    virtual ~StringUtils();
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
    static std::string hexMD5(const char *key);

private:
    HashUtils();
    virtual ~HashUtils();
};

template<typename Cont> inline std::string
StringUtils::urlencode(const Cont &cont) {
    return urlencode(createRange(cont));
}

template<typename Cont> inline std::string
StringUtils::urldecode(const Cont &cont) {
    return urldecode(createRange(cont));
}

template<typename Cont> inline void
StringUtils::parse(const Cont &cont, std::vector<NamedValue> &v, Encoder *encoder) {
    parse(createRange(cont), v, encoder);
}

class TimeoutCounter {
public:
    TimeoutCounter();
    TimeoutCounter(int timeout);
    ~TimeoutCounter();

    void reset(int timeout);
    bool unlimited() const;
    bool expired() const;
    int remained() const;

    static const int UNDEFINED_TIME;
private:
    struct timeval init_time_;
    int timeout_;
};

void terminate(int status, const char* message, bool write_log);

} // namespace xscript

#endif // _XSCRIPT_UTIL_H_
