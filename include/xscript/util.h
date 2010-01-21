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
#include <xscript/logger.h>
#include <xscript/range.h>
#include <xscript/xml_helpers.h>

namespace xscript {

class HttpDateUtils : private boost::noncopyable {
public:
    static time_t parse(const char *value);
    static std::string format(time_t value);
    static boost::int32_t expires(boost::int32_t delta);
    
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
    
    static void encodeBase64(const char *input, unsigned long len, std::string &result);
    static void decodeBase64(const char *input, unsigned long len, std::string &result);
private:
    HashUtils();
    virtual ~HashUtils();
};

class FileUtils : private boost::noncopyable {
public:
    static std::string normalize(const std::string &filepath);
    static bool fileExists(const std::string &name);
    static time_t modified(const std::string &name);
    static void makeDir(const std::string &name, mode_t mode);

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
