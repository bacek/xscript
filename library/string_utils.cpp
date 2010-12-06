#include <libxml/tree.h>
#include <stdexcept>
#include <memory>
#include <algorithm>

#include "xscript/algorithm.h"
#include "xscript/encoder.h"
#include "xscript/string_utils.h"
#include "xscript/xml_util.h"

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

std::string
StringUtils::urlencode(const Range &range, bool force_percent) {

    std::string result;
    result.reserve(3 * range.size());

    for (const char* i = range.begin(), *end = range.end(); i != end; ++i) {
        char symbol = (*i);
        if (!force_percent && isalnum(symbol)) {
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
            if (!force_percent) {
                result.append(1, symbol);
                break;
            }
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
        xscript::split(head, '=', key, value);
        if (!key.empty()) {
            std::pair<std::string, std::string> p(urldecode(key), urldecode(value));
            if (!xmlCheckUTF8((const xmlChar*) p.first.c_str())) {
                encoder->encode(p.first).swap(p.first);
            }
            if (!XmlUtils::validate(p.first)) {
                throw std::runtime_error("Data is not allowed in query: " + p.first);
            }
            if (!xmlCheckUTF8((const xmlChar*) p.second.c_str())) {
                encoder->encode(p.second).swap(p.second);
            }
            if (!XmlUtils::validate(p.second)) {
                throw std::runtime_error("Data is not allowed in query: " + p.second);
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

std::string
StringUtils::parseDomainFromURL(std::string url, boost::int32_t level) {
    
    if (0 > level) {
        throw std::invalid_argument("bad param: level");
    }

    std::string::size_type pos;
    pos = url.find('?');
    if (std::string::npos != pos) {
        url.erase(pos);
    }

    pos = url.find("://");
    if (std::string::npos != pos) {
        url.erase(0, pos + 3);
    }

    pos = std::min(url.find('/'), url.find(':'));

    if (std::string::npos != pos) {
        url.erase(pos);
    }

    if (url.empty() || '.' == *url.begin() || '.' == *url.rbegin()) {
        throw std::invalid_argument("bad param: domain='" + url + "'");
    }

    boost::int32_t max = std::count(url.begin(), url.end(), '.');
    if (0 == level) {
        level = max + 1;
    }

    char c = url[url.rfind('.') + 1];
    if (c >= '0' && c <= '9') {
        throw std::invalid_argument("bad param: domain='" + url + "'");
    }

    //if (max < level - 1) {
    //    log()->warn("max available domain level is less than required mist:set_state_domain");
    //}
    if (max) {
        std::string::size_type end = std::string::npos, tmp = 0;
        for (boost::int32_t i = 0; i <= max; ++i) {
            pos = url.rfind('.', --tmp);
            if (tmp == pos) {
                throw std::invalid_argument("bad param: domain='" + url + "'");
            }
            tmp = pos;
            if (i < level) {
                end = pos + 1;
            }
        }
        if (end) {
            url.erase(0, end);
        }
    }

    return url;
}

void
StringUtils::split(const std::string &val, const std::string &delim, std::vector<std::string> &v) {
    if (delim.empty() || delim[0] == '\0') {
        throw std::runtime_error("empty delimeter");
    }

    bool searching = true;
    std::string::size_type pos, lpos = 0;
    while (searching) {
        if ((pos = val.find(delim, lpos)) == std::string::npos) {
            searching = false;
        }
        v.push_back(val.substr(lpos, searching ? pos - lpos : std::string::npos));
        lpos = pos + delim.size();
    }
}

}
