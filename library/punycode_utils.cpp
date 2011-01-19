#include "settings.h"

#include <ctype.h>
#include <punycode.h>

#include <sstream>
#include <stdexcept>

#include <boost/tokenizer.hpp>

#include "xscript/encoder.h"
#include "xscript/punycode_utils.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

PunycodeEncoder::PunycodeEncoder() :
    to_ucs4_(Encoder::createStrict("UTF-8", "UCS-4LE"))
{
}

PunycodeEncoder::PunycodeEncoder(const char *from) :
    to_ucs4_(Encoder::createStrict(from, "UCS-4LE"))
{
}

PunycodeEncoder::~PunycodeEncoder()
{
}

void
PunycodeEncoder::encode(const Range &val, std::string &dest) const {

    std::string dst;
    to_ucs4_->encode(val, dst);

    std::string puny_out;
    puny_out.resize(5 + dst.size() * sizeof(punycode_uint));

    for (;;) {
        size_t out_len = puny_out.size();
        const int status = punycode_encode(
            dst.size() * sizeof(char) / sizeof(punycode_uint),
            reinterpret_cast<const punycode_uint*>(dst.c_str()), NULL, &out_len, &puny_out[0]);

        if (status == punycode_success) {
    	    puny_out.resize(out_len);
	    dest.swap(puny_out);
    	    return;
        }
        if (status == punycode_big_output) {
            puny_out.resize(puny_out.size() * 2);
            continue;
        }
        std::stringstream stream;
        stream << "punycode encoder error: " << punycode_strerror((Punycode_status)status);
        throw std::runtime_error(stream.str());
    }
}

void
PunycodeEncoder::domainEncode(const Range &val, std::string &dest) const {

    typedef boost::char_separator<char> Separator;
    typedef boost::tokenizer<Separator> Tokenizer;

    std::string res;
    res.reserve(val.size() * 2 + 20);

    Tokenizer tok(val, Separator("", ".", boost::drop_empty_tokens));
    for (Tokenizer::const_iterator it = tok.begin(), it_end = tok.end(); it != it_end; ++it) {

        const std::string &token = *it;
        if ("." == token) {
            res.push_back('.');
            continue;
        }

        bool need_encode = false;
        for (const char *s = token.c_str(); *s; ++s) {
            if (*s != '-' && !::isalnum(*s)) {
                need_encode = true;
                break;
            }
        }
        if (need_encode) {
            std::string dst;
	    encode(createRange(token), dst);
            res.append("xn--").append(dst);
        }
        else {
            res.append(token);
        }
    }
    dest.swap(res);
}


PunycodeDecoder::PunycodeDecoder() :
    from_ucs4_(Encoder::createStrict("UCS-4LE", "UTF-8"))
{
}

PunycodeDecoder::PunycodeDecoder(const char *to) :
    from_ucs4_(Encoder::createStrict("UCS-4LE", to))
{
}

PunycodeDecoder::~PunycodeDecoder() {
}

void
PunycodeDecoder::decode(const Range &val, std::string &dest) const {

    std::vector<punycode_uint> puny_out;
    puny_out.resize(5 + val.size());

    for (;;) {
        size_t out_len = puny_out.size();
        punycode_uint *pout = &puny_out[0];
        const int status = punycode_decode(
            val.size(), val.begin(), &out_len, pout, NULL);
        if (status == punycode_success) {
    	    Range range((const char*)pout, (const char*)(&pout[out_len]));
	    from_ucs4_->encode(range, dest);
    	    return;
        }
        if (status == punycode_big_output) {
            puny_out.resize(puny_out.size() * 2);
            continue;
        }
        std::stringstream stream;
        stream << "punycode decoder error: " << punycode_strerror((Punycode_status)status);
        throw std::runtime_error(stream.str());
    }
}

void
PunycodeDecoder::domainDecode(const Range &val, std::string &dest) const {

    typedef boost::char_separator<char> Separator;
    typedef boost::tokenizer<Separator> Tokenizer;

    std::string res;
    res.reserve(val.size() * 2);

    Tokenizer tok(val, Separator("", ".", boost::drop_empty_tokens));
    for (Tokenizer::const_iterator it = tok.begin(), it_end = tok.end(); it != it_end; ++it) {

        const std::string &token = *it;
        if ("." == token) {
            res.push_back('.');
            continue;
        }

        const char *str = token.c_str();
        if (!strncasecmp(str, "xn--", sizeof("xn--") - 1)) {
            std::string dst;
	    Range range(str + sizeof("xn--") - 1, str + token.size());
	    decode(range, dst);
            res.append(dst);
        }
        else {
            res.append(token);
        }
    }
    dest.swap(res);
}

}
