#include "settings.h"

#include <cctype>
#include <cstdarg>
#include <cassert>
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
#include "xscript/xml_util.h"
#include "xscript/logger.h"
#include "xscript/encoder.h"

#include "internal/algorithm.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

static const int NEXT_UTF8[256] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
};

const std::string StringUtils::EMPTY_STRING;

std::string
StringUtils::urlencode(const Range &range) {

    std::string result;
    result.reserve(3 * range.size());

    for (const char* i = range.begin(), *end = range.end(); i != end; ++i) {
        char symbol = (*i);
        if (isalnum(symbol)) {
            result.append(1, symbol);
            continue;
        }
        switch (symbol) {
        case '-':
        case '_':
        case '.':
        case '!':
        case '~':
        case '*':
        case '(':
        case ')':
        case '\'':
            result.append(1, symbol);
            break;
        default:
            result.append(1, '%');
            char bytes[3] = { 0, 0, 0 };
            bytes[0] = (symbol & 0xF0) / 16 ;
            bytes[0] += (bytes[0] > 9) ? 'A' - 10 : '0';
            bytes[1] = symbol & 0x0F;
            bytes[1] += (bytes[1] > 9) ? 'A' - 10 : '0';
            result.append(bytes, sizeof(bytes) - 1);
            break;
        }
    }
    return result;
}

std::string
StringUtils::urldecode(const Range &range) {

    std::string result;
    result.reserve(range.size());

    for (const char *i = range.begin(), *end = range.end(); i != end; ++i) {
        switch (*i) {
        case '+':
            result.push_back(' ');
            break;
        case '%':
            if (std::distance(i, end) > 2) {
                int digit;
                char f = *(i + 1), s = *(i + 2);
                digit = (f >= 'A' ? ((f & 0xDF) - 'A') + 10 : (f - '0')) * 16;
                digit += (s >= 'A') ? ((s & 0xDF) - 'A') + 10 : (s - '0');
                if (digit == 0) {
                    throw std::runtime_error("Null symbol in URL is not allowed");
                }
                result.push_back(static_cast<char>(digit));
                i += 2;
            }
            else {
                result.push_back('%');
            }
            break;
        default:
            result.push_back(*i);
            break;
        }
    }
    return result;
}

void
StringUtils::parse(const Range &range, std::vector<NamedValue> &v, Encoder *encoder) {

    Range part = range, key, value, head, tail;
    std::auto_ptr<Encoder> enc(NULL);
    if (NULL == encoder) {
        enc = Encoder::createDefault("cp1251", "utf-8");
        encoder = enc.get();
    }
    while (!part.empty()) {
        splitFirstOf(part, "&;", head, tail);
        split(head, '=', key, value);
        if (!key.empty()) {
            std::pair<std::string, std::string> p(urldecode(key), urldecode(value));
            if (!xmlCheckUTF8((const xmlChar*) p.first.c_str())) {
                encoder->encode(p.first).swap(p.first);
            }
            if (!xmlCheckUTF8((const xmlChar*) p.second.c_str())) {
                encoder->encode(p.second).swap(p.second);
            }
            v.push_back(p);
        }
        part = tail;
    }
}

const char*
wrapStrerror(int, const char *value) {
    return value;
}

const char*
wrapStrerror(const char *value, const char *) {
    return value;
}

void
StringUtils::report(const char *what, int error, std::ostream &stream) {
    char buffer[256];
    stream << what << wrapStrerror(strerror_r(error, buffer, sizeof(buffer)), buffer);
}

std::string
StringUtils::tolower(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    int (*func)(int) = &std::tolower;
    std::transform(str.begin(), str.end(), std::back_inserter(result), func);
    return result;
}

std::string
StringUtils::toupper(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    int (*func)(int) = &std::toupper;
    std::transform(str.begin(), str.end(), std::back_inserter(result), func);
    return result;
}

const char*
StringUtils::nextUTF8(const char* data) {
    return data + NEXT_UTF8[static_cast<unsigned char>(*data)];
}

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
    }
    throw std::runtime_error("format date failed");
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

    struct timeval current;
    gettimeofday(&current, 0);

    return timeout_ - (current.tv_sec - init_time_.tv_sec) * 1000 -
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
        if (info_.end() != it) {
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
