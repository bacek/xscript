#include "settings.h"

#include <cerrno>
#include <cstdio>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <boost/cstdint.hpp>

#include "xscript/util.h"
#include "xscript/range.h"
#include "xscript/encoder.h"
#include "xscript/string_utils.h"
#include "internal/expect.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

const iconv_t IconvTraits::DEFAULT_VALUE = reinterpret_cast<iconv_t>(-1);

struct EncoderContext {
    size_t len;
    ICONV_CONST char *data;
    ICONV_CONST char *begin;

    void throwBadSymbolError() const;
};

class DefaultEncoder : public Encoder {
public:
    DefaultEncoder(const char *from, const char *to);
    virtual ~DefaultEncoder();

protected:
    virtual size_t rep(char *buf, size_t size, const EncoderContext &ctx) const;
};

class IgnoringEncoder : public Encoder {
public:
    IgnoringEncoder(const char *from, const char *to);
    virtual ~IgnoringEncoder();

protected:
    virtual size_t rep(char *buf, size_t size, const EncoderContext &ctx) const;

};

class StrictEncoder : public Encoder {
public:
    StrictEncoder(const char *from, const char *to);
    virtual ~StrictEncoder();

protected:
    virtual size_t rep(char *buf, size_t size, const EncoderContext &ctx) const;
};

class EscapingEncoder : public Encoder {
public:
    EscapingEncoder(const char *from, const char *to);
    virtual ~EscapingEncoder();

protected:
    virtual size_t rep(char *buf, size_t size, const EncoderContext &ctx) const;

private:
    IconvHolder escaper_;
};


void
EncoderContext::throwBadSymbolError() const {
    std::stringstream stream;
    stream << "bad symbol at pos " << (data - begin);
    throw std::runtime_error(stream.str());
}


void
IconvTraits::destroy(iconv_t conv) {
    iconv_close(conv);
}

Encoder::Encoder(const char *from, const char *to) :
        from_(from) {
    iconv_ = IconvHolder(iconv_open(to, from));
    check(iconv_);
}

Encoder::~Encoder() {
}

std::auto_ptr<Encoder>
Encoder::createDefault(const char *from, const char *to) {
    return std::auto_ptr<Encoder>(new DefaultEncoder(from, to));
}

std::auto_ptr<Encoder>
Encoder::createIgnoring(const char *from, const char *to) {
    return std::auto_ptr<Encoder>(new IgnoringEncoder(from, to));
}

std::auto_ptr<Encoder>
Encoder::createEscaping(const char *from, const char *to) {
    return std::auto_ptr<Encoder>(new EscapingEncoder(from, to));
}

std::auto_ptr<Encoder>
Encoder::createStrict(const char *from, const char *to) {
    return std::auto_ptr<Encoder>(new StrictEncoder(from, to));
}

void
Encoder::encode(const Range &range, std::string &dst) const {

    if (XSCRIPT_UNLIKELY(0 == range.size())) {
        return;
    }

    std::vector<char> v(range.size() * 2);
    size_t rsize = range.size(), dsize = v.size();
    
    char *dch = &v[0];
    ICONV_CONST char *sch = const_cast<ICONV_CONST char*>(range.begin());

    while (rsize != 0) {
        size_t res = iconv(iconv_.get(), &sch, &rsize, &dch, &dsize);
        if (static_cast<size_t>(-1) == res) {
            if (E2BIG == errno) {
                size_t tmp = v.size() - dsize;
                dsize += v.size();
                v.resize(v.size() * 2);
                dch = &v[0] + tmp;
            }
            else if (EILSEQ == errno) {

                EncoderContext ctx = { 0, sch, &v[0] };

                char buf[16];
                ICONV_CONST char *n = const_cast<ICONV_CONST char*>(next(ctx));
                size_t tmp = n - sch;
                if (tmp > rsize) {
                    ctx.throwBadSymbolError();
                }
                size_t cur = v.size() - dsize;

                ctx.len = tmp;
                size_t len = rep(buf, sizeof(buf), ctx);
                v.insert(v.begin() + cur, buf, buf + len);
                dch = &v[0] + cur + len;

                rsize -= tmp;
                sch = n;
            }
            else {
                std::stringstream stream;
                StringUtils::report("encoder error: ", errno, stream);
                throw std::runtime_error(stream.str());
            }
        }
    }
    dst.assign(&v[0], dch);
}

void
Encoder::check(const IconvHolder &conv) const {
    if (IconvTraits::DEFAULT_VALUE == conv.get()) {
        std::stringstream stream;
        StringUtils::report("encoder error: ", errno, stream);
        throw std::runtime_error(stream.str());
    }
}

const char*
Encoder::next(const EncoderContext &ctx) const {

    ICONV_CONST char *data = ctx.data;
    if (!strncasecmp(from_.c_str(), "utf-8", sizeof("utf-8"))) {
        return StringUtils::nextUTF8(data);
    }
    if (!strncasecmp(from_.c_str(), "utf-16", sizeof("utf-16") - 1)) {
        return data + 2;
    }
    if (!strncasecmp(from_.c_str(), "ucs-4", sizeof("ucs-4") - 1)) {
        return data + 4;
    }
    std::stringstream stream;
    stream << "can not find next character at pos " << (ctx.data - ctx.begin) << std::endl;
    throw std::runtime_error(stream.str());
}

DefaultEncoder::DefaultEncoder(const char *from, const char *to) :
        Encoder(from, to) {

}

DefaultEncoder::~DefaultEncoder() {
}

size_t
DefaultEncoder::rep(char *buf, size_t size, const EncoderContext & /* ctx */) const {
    return snprintf(buf, size, "?");
}

IgnoringEncoder::IgnoringEncoder(const char *from, const char *to) :
        Encoder(from, to) {
}

IgnoringEncoder::~IgnoringEncoder() {
}

size_t
IgnoringEncoder::rep(char * /* buf */, size_t /* size */, const EncoderContext &/* ctx */) const {
    return 0;
}

StrictEncoder::StrictEncoder(const char *from, const char *to) :
        Encoder(from, to) {
}

StrictEncoder::~StrictEncoder() {
}

size_t
StrictEncoder::rep(char * /* buf */, size_t /* size */, const EncoderContext &ctx) const {
    ctx.throwBadSymbolError();
    return 0;
}

EscapingEncoder::EscapingEncoder(const char *from, const char *to) :
        Encoder(from, to) {
    escaper_ = IconvHolder(iconv_open("UCS-4LE", from)); //UTF-16LE
    check(escaper_);
}

EscapingEncoder::~EscapingEncoder() {
}

size_t
EscapingEncoder::rep(char *buf, size_t size, const EncoderContext &ctx) const {

    if (0 == ctx.len) {
        return 0;
    }

    ICONV_CONST char *ch = ctx.data;
    size_t len = ctx.len;
    boost::uint32_t value;
    size_t vlen = sizeof(value);
    char *dch = reinterpret_cast<char*>(&value);

    size_t res = iconv(escaper_.get(), &ch, &len, &dch, &vlen);
    if (0 != len) {
        ctx.throwBadSymbolError();
    }
    if (static_cast<size_t>(-1) == res) {
        std::stringstream stream;
        StringUtils::report("encoder error", errno, stream);
        throw std::runtime_error(stream.str());
    }
    return snprintf(buf, size, "&#%u;", value);
}

} // namespace yandex
