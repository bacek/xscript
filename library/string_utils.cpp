#include "settings.h"

#include <libxml/tree.h>
#include <stdexcept>
#include <memory>
#include <algorithm>

#include "xscript/algorithm.h"
#include "xscript/encoder.h"
#include "xscript/string_utils.h"
#include "xscript/xml_util.h"

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

static bool ESC_CHECK_CHARSET[256] = { false };
static bool JS_CHECK_CHARSET[256] = { false };

struct StringUtilsInit {
    StringUtilsInit() {
        for (const char *s1 = "\\/-'\""; *s1; ++s1) {
            ESC_CHECK_CHARSET[*(const unsigned char *)s1] = true;
        }
        for (const char *s2 = "\\/-'"; *s2; ++s2) {
            JS_CHECK_CHARSET[*(const unsigned char *)s2] = true;
        }
    }
};

static StringUtilsInit string_utils_init_;


void
StringUtils::jsonQuoteString(const Range &range, std::string &result) {

    const char *str = range.begin();
    const char *end = range.end();
    while (str < end) {
        char ch = *str;
        switch(ch) {
            case '"':
            case '/':
                result.push_back('\\');
                break;
            case '\b':
                result.push_back('\\');
                ch = 'b';
                break;
            case '\f':
                result.push_back('\\');
                ch = 'f';
                break;
            case '\n':
                result.push_back('\\');
                ch = 'n';
                break;
            case '\r':
                result.push_back('\\');
                ch = 'r';
                break;
            case '\t':
                result.push_back('\\');
                ch = 't';
                break;
            case '\v':
                result.push_back('\\');
                ch = 'v';
                break;
            case '\\':
                result.push_back('\\');
                if (end - str > 5 && str[1] == 'u' &&
                    isxdigit(str[2]) && isxdigit(str[3]) &&
                    isxdigit(str[4]) && isxdigit(str[5])) {

                    result.append(str, 6);
                    str += 6;
                    continue;
                }
            default:
                break;
        }
        result.push_back(ch);
        ++str;
    }
}

inline static void
escapeTemplatedString(const Range &range, const bool * const esc_template_check, bool js_check, std::string &result) {

    const char *str = range.begin();
    const char *end = range.end();

    const char *str_next = str;
    while (str_next < end) {
        str = str_next;
        unsigned char ch = *(const unsigned char*)str;
        const int char_size = NEXT_UTF8[ch]; //StringUtils::nextUTF8(str) - str;
        str_next = str + char_size;
        if (ch == '\n') {
            result.append("\\n", 2);
            continue;
        }
        if (ch == '\r') {
            result.append("\\n", 2);
            if (end - str > 1 && str[1] == '\n') {
                ++str_next;
            }
            continue;
        }
        //if (char_size == 1 && NULL != strchr(esc_template, ch)) {
        if (char_size == 1 && esc_template_check[ch]) {
            if (!js_check || ch != '-' || (str > range.begin() && str[-1] == '-')) {
                result.push_back('\\');
            }
        }
        else if (js_check && char_size > 1) {
            if (char_size == 2) {
                if (ch == 0xC2 && str[1] == '\x85') {
                    result.append("\\u0085", 6);
                    continue;
                }
            }
            else if (char_size == 3) {
                if (ch == 0xE2 && str[1] == '\x80') {
                    if (str[2] == '\xA8') {
                        result.append("\\u2028", 6);
                        continue;
                    }
                    else if (str[2] == '\xA9') {
                        result.append("\\u2029", 6);
                        continue;
                    }
                }
            }
        }
        result.append(str, str_next - str);
    }
}

void
StringUtils::escapeString(const Range &range, std::string &result) {
    if (!range.empty()) {
        //escapeTemplatedString(range, "\\/-'\"", false, result);
        escapeTemplatedString(range, ESC_CHECK_CHARSET, false, result);
    }
}

void
StringUtils::jsQuoteString(const Range &range, std::string &result) {
    if (!range.empty()) {
        //escapeTemplatedString(range, "\\/-'", true, result);
        escapeTemplatedString(range, JS_CHECK_CHARSET, true, result);
    }
}

std::string
StringUtils::urlencode(const Range &range, bool force_percent) {

    if (range.empty()) {
        return EMPTY_STRING;
    }

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

    if (range.empty()) {
        return EMPTY_STRING;
    }

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
        throw std::runtime_error("empty delimiter");
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
