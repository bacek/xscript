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
#include "details/expect.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif 

namespace xscript
{

const iconv_t IconvTraits::DEFAULT_VALUE = reinterpret_cast<iconv_t>(-1);

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

struct EncoderContext
{
	size_t len;
	char *data, *begin;
};

class DefaultEncoder : public Encoder
{
public:
	DefaultEncoder(const char *from, const char *to);
	virtual ~DefaultEncoder();

protected:
	size_t rep(char *buf, size_t size, const EncoderContext &ctx) const;
	
private:
	DefaultEncoder(const DefaultEncoder &);
	DefaultEncoder& operator = (const DefaultEncoder &);
};

class IgnoringEncoder : public Encoder
{
public:
	IgnoringEncoder(const char *from, const char *to);
	virtual ~IgnoringEncoder();

protected:
	size_t rep(char *buf, size_t size, const EncoderContext &ctx) const;
	
private:
	IgnoringEncoder(const IgnoringEncoder &);
	IgnoringEncoder& operator = (const IgnoringEncoder &);
};

class EscapingEncoder : public Encoder
{
public:
	EscapingEncoder(const char *from, const char *to);
	virtual ~EscapingEncoder();
	
protected:
	size_t rep(char *buf, size_t size, const EncoderContext &ctx) const;
	
private:
	EscapingEncoder(const EscapingEncoder &);
	EscapingEncoder& operator = (const EscapingEncoder &);
	
private:
	IconvHelper escaper_;
};

void
IconvTraits::clean(iconv_t conv) {
	iconv_close(conv);
}

Encoder::Encoder(const char *from, const char *to) : 
	from_(from)
{
	iconv_ = IconvHelper(iconv_open(to, from));
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

void
Encoder::encode(const Range &range, std::string &dst) const {
	
	if (XSCRIPT_UNLIKELY(0 == range.size())) {
		return;
	}

	std::vector<char> v(range.size());
	size_t rsize = range.size(), dsize = v.size();
	char *sch = const_cast<char*>(range.begin()), *dch = &v[0];
	
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
			
				char *n = next(ctx), buf[16];
				size_t tmp = n - sch;
				size_t cur = v.size() - dsize;
				
				ctx.len = n - sch;
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
Encoder::check(const IconvHelper &conv) const {
	if (IconvTraits::DEFAULT_VALUE == conv.get()) {
		std::stringstream stream;
		StringUtils::report("encoder error: ", errno, stream);
		throw std::runtime_error(stream.str());
	}
}

char*
Encoder::next(const EncoderContext &ctx) const {
	
	char *data = ctx.data;
	if (strncasecmp(from_.c_str(), "utf-8", sizeof("utf-8")) == 0) {
		return data + NEXT_UTF8[static_cast<unsigned char>(*data)];
	}
	else if (strncasecmp(from_.c_str(), "utf-16", sizeof("utf-16") - 1) == 0) {
		return data + 2;
	}
	else {
		std::stringstream stream;
		stream << "can not find next character at pos " << (ctx.data - ctx.begin) << std::endl;
		throw std::runtime_error(stream.str());
	}
}

DefaultEncoder::DefaultEncoder(const char *from, const char *to) :
	Encoder(from, to)
{

}

DefaultEncoder::~DefaultEncoder() {
}

size_t
DefaultEncoder::rep(char *buf, size_t size, const EncoderContext &ctx) const {
	return snprintf(buf, size, "?");
}

IgnoringEncoder::IgnoringEncoder(const char *from, const char *to) :
	Encoder(from, to)
{
}

IgnoringEncoder::~IgnoringEncoder() {
}

size_t
IgnoringEncoder::rep(char *buf, size_t size, const EncoderContext &ctx) const {
	return 0;
}

EscapingEncoder::EscapingEncoder(const char *from, const char *to) :
	Encoder(from, to)
{
	escaper_ = IconvHelper(iconv_open("UTF-16LE", from));
	check(escaper_);
}

EscapingEncoder::~EscapingEncoder() {
}

size_t
EscapingEncoder::rep(char *buf, size_t size, const EncoderContext &ctx) const {
	
	if (0 != ctx.len) {

		char *ch = ctx.data;
		size_t len = ctx.len;
		std::vector<boost::uint16_t> v(len);
		size_t vlen = len * sizeof(boost::uint16_t);;
		char *dch = reinterpret_cast<char*>(&v[0]);
		
		size_t res = iconv(escaper_.get(), &ch, &len, &dch, &vlen);
		if (0 != len) {
			std::stringstream stream;
			stream << "bad symbol at pos " << (ctx.data - ctx.begin);
			throw std::runtime_error(stream.str());
		}
		if (static_cast<size_t>(-1) == res) {
			std::stringstream stream;
			StringUtils::report("encoder error", errno, stream);
			throw std::runtime_error(stream.str());
		}
		return snprintf(buf, size, "&#%u;", static_cast<unsigned int>(v[0]));
	}
	return 0;
}

} // namespace yandex
