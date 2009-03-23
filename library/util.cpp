#include "settings.h"

#include <cctype>
#include <cstdarg>
#include <cassert>
#include <cerrno>
#include <stdexcept>
#include <iostream>

#include <boost/crc.hpp>
#include <boost/cstdint.hpp>
#include <boost/current_function.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <openssl/blowfish.h>
#include <openssl/md5.h>

#include "xscript/util.h"
#include "xscript/string_utils.h"
#include "xscript/xml_util.h"
#include "xscript/logger.h"
#include "xscript/encoder.h"

#include "internal/algorithm.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

HttpDateUtils::HttpDateUtils() {
}

HttpDateUtils::~HttpDateUtils() {
}

static const char httpDateUtilsShortWeekdays [7] [4] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
static const char httpDateUtilsShortMonths [12] [4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

std::string
HttpDateUtils::format(time_t value) {

    struct tm ts;
    memset(&ts, 0, sizeof(struct tm));

    if (NULL != gmtime_r(&value, &ts)) {
        char buf[255];

        //int res = strftime(buf, sizeof(buf), "%a, %d %b %Y %T GMT", &ts);
        int res = snprintf(buf, sizeof(buf) - 1, "%s, %02d %s %04d %02d:%02d:%02d GMT",
                           httpDateUtilsShortWeekdays[ts.tm_wday],
                           ts.tm_mday, httpDateUtilsShortMonths[ts.tm_mon], ts.tm_year + 1900,
                           ts.tm_hour, ts.tm_min, ts.tm_sec);

        if (0 < res) {
            return std::string(buf, buf + res);
        }
        else {
            std::stringstream stream;
            StringUtils::report("format date failed: ", errno, stream);
            throw std::runtime_error(stream.str());
        }
    }
    throw std::runtime_error("format date failed. Value: " + boost::lexical_cast<std::string>(value));
}

time_t
HttpDateUtils::parse(const char *value) {

    struct tm ts;
    memset(&ts, 0, sizeof(struct tm));

    const char *formats[] = { "%a, %d %b %Y %T GMT", "%A, %d-%b-%y %T GMT", "%a %b %d %T %Y" };
    for (unsigned int i = 0; i < sizeof(formats)/sizeof(const char*); ++i) {
        if (NULL != strptime(value, formats[i], &ts)) {
            return timegm(&ts); // mktime(&ts) - timezone;
        }
    }
    return static_cast<time_t>(0);
}

HashUtils::HashUtils() {
}

HashUtils::~HashUtils() {
}

std::string
HashUtils::hexMD5(const char *key, unsigned long len) {

    MD5_CTX md5handler;
    unsigned char md5buffer[16];

    MD5_Init(&md5handler);
    MD5_Update(&md5handler, (unsigned char *)key, len);
    MD5_Final(md5buffer, &md5handler);

    char alpha[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
    unsigned char c;
    std::string md5digest;
    md5digest.reserve(32);

    for (int i = 0; i < 16; ++i) {
        c = (md5buffer[i] & 0xf0) >> 4;
        md5digest.push_back(alpha[c]);
        c = (md5buffer[i] & 0xf);
        md5digest.push_back(alpha[c]);
    }

    return md5digest;
}

std::string
HashUtils::hexMD5(const char *key) {
    return hexMD5(key, strlen(key));
}

std::string
HashUtils::hexMD5(const ByteArrayType &key) {
    return hexMD5(&*key.begin(), key.size());
}

std::string
HashUtils::blowfish(const char *data, unsigned long data_len,
                    const char *key, unsigned long key_len, const char *ivec) {

    unsigned char ivec_buffer[8];
    memset(ivec_buffer, 0, 8);
    memcpy(ivec_buffer, ivec, std::min(strlen(ivec), (size_t)8));

    BF_KEY bfkey;

    BF_set_key(&bfkey, key_len, (unsigned char *)key);

    size_t padded_datasize = (data_len % 8) ? 8*(data_len/8 + 1) : data_len;

    unsigned char buffer[padded_datasize];

    if (padded_datasize > data_len) {
        ByteArrayType input;
        input.reserve(padded_datasize);
        input.insert(input.end(), data, data + data_len);
        input.insert(input.end(), padded_datasize - data_len, '\0');
        BF_cbc_encrypt((unsigned char *)&*input.begin(),
            (unsigned char *)buffer,
            padded_datasize,
            &bfkey,
            (unsigned char *)ivec_buffer,
            BF_ENCRYPT);
    }
    else {
        BF_cbc_encrypt((unsigned char *)data,
            (unsigned char *)buffer,
            padded_datasize,
            &bfkey,
            (unsigned char *)ivec_buffer,
            BF_ENCRYPT);
    }

    return std::string(buffer, buffer + padded_datasize);
}

std::string
HashUtils::blowfish(const char *data, const char *key, const char *ivec) {
    return blowfish(data, strlen(data), key, strlen(key), ivec);
}

std::string
HashUtils::blowfish(const ByteArrayType &data, const ByteArrayType &key, const char *ivec) {
    return blowfish(&*data.begin(), data.size(), &*key.begin(), key.size(), ivec);
}

boost::uint32_t
HashUtils::crc32(const std::string &key) {
    return crc32(key.data(), key.size());
}

boost::uint32_t
HashUtils::crc32(const char *key, unsigned long len) {
    boost::crc_32_type result;
    result.process_bytes(key, len);
    return result.checksum();
}

FileUtils::FileUtils() {
}

FileUtils::~FileUtils() {
}

std::string
FileUtils::normalize(const std::string &filepath) {

    namespace fs = boost::filesystem;

#if BOOST_VERSION < 103401
    std::string res(filepath);
    std::string::size_type length = res.length();
    for (std::string::size_type i = 0; i < length - 1; ++i) {
        if (res[i] == '/' && res[i + 1] == '/') {
            res.erase(i, 1);
            --i;
            --length;
        }
    }
    fs::path path(res);
#else
    fs::path path(filepath);
#endif

    path.normalize();
    return path.string();
}

bool
FileUtils::fileExists(const std::string &name) {
    namespace fs = boost::filesystem;
    fs::path path(name);
    return fs::exists(path) && !fs::is_directory(path);
}

time_t
FileUtils::modified(const std::string &name) {
    namespace fs = boost::filesystem;
    fs::path path(name);
    return fs::last_write_time(path);
}

const int TimeoutCounter::UNLIMITED_TIME = std::numeric_limits<int>::max();

TimeoutCounter::TimeoutCounter() {
    reset(UNLIMITED_TIME);
}

TimeoutCounter::TimeoutCounter(int timeout) {
    reset(timeout);
}

TimeoutCounter::~TimeoutCounter() {
}

void
TimeoutCounter::timeout(int timeout) {
    timeout_ = timeout;
}

int
TimeoutCounter::timeout() const {
    return timeout_;
}

void
TimeoutCounter::reset(int timeout) {
    if (timeout <= 0) {
        timeout_ = 0;
    }
    else {
        timeout_ = timeout;
    }
    gettimeofday(&init_time_, 0);
}

int
TimeoutCounter::remained() const {
    if (unlimited()) {
        return UNLIMITED_TIME;
    }
    return timeout_ - elapsed();
}

int
TimeoutCounter::elapsed() const {
    struct timeval current;
    gettimeofday(&current, 0);
    return (current.tv_sec - init_time_.tv_sec) * 1000 +
           (current.tv_usec - init_time_.tv_usec) / 1000;
}

bool
TimeoutCounter::unlimited() const {
    return timeout_ == UNLIMITED_TIME;
}

bool
TimeoutCounter::expired() const {
    if (unlimited()) {
        return false;
    }
    return remained() <= 0;
}

void terminate(int status, const char* message, bool write_log) {
    std::cerr << "Xscript is terminating: " << message << std::endl;
    if (write_log) {
        log()->crit("Xscript is terminating: %s", message);
    }
    exit(status);
}

InvokeError::InvokeError(const std::string &error, const std::string &name, const std::string &value) :
    UnboundRuntimeError(error) {
    addEscaped(name, value);
}

void
InvokeError::add(const std::string &name, const std::string &value) {
    if (name.empty()) {
        throw std::runtime_error("empty key");
    }
    if (!value.empty()) {
        std::pair<std::string, std::string> info = std::make_pair(name, value);
        if (info_.empty()) {
            info_.push_back(info);
            return;
        }
        InfoMapType::iterator it = std::find(info_.begin(), info_.end(), info);
        if (info_.end() == it) {
            info_.push_back(info);
        }
    }
}

void
InvokeError::addEscaped(const std::string &name, const std::string &value) {
    add(name, XmlUtils::escape(value));
}

std::string
InvokeError::what_info() const throw() {

    std::stringstream stream;
    stream << what() << ". ";

    for(InvokeError::InfoMapType::const_iterator it = info_.begin();
        it != info_.end();
        ++it) {
        stream << it->first << ": " << it->second << ". ";
    }

    return stream.str();
}

} // namespace xscript
