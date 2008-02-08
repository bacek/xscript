#ifndef _XSCRIPT_ENCODER_H_
#define _XSCRIPT_ENCODER_H_

#include <string>
#include <memory>
#include <boost/utility.hpp>
#include <xscript/range.h>
#include <xscript/resource_holder.h>
#include <iconv.h>

namespace xscript
{

struct IconvTraits
{
	static void destroy(iconv_t conv);
	static const iconv_t DEFAULT_VALUE;
};

typedef ResourceHolder<iconv_t, IconvTraits> IconvHolder;

struct EncoderContext;

class Encoder : private boost::noncopyable
{
public:

	inline std::string encode(const Range &range) const {
		std::string dest;
		encode(range, dest);
		return dest;
	}
	
	template<typename Cont> std::string encode(const Cont &cont) const;
	
	void encode(const Range &range, std::string &dst) const;
	template<typename Cont> void encode(const Cont &cont, std::string &dest) const;

	static std::auto_ptr<Encoder> createDefault(const char *from, const char *to);
	static std::auto_ptr<Encoder> createIgnoring(const char *from, const char *to);
	static std::auto_ptr<Encoder> createEscaping(const char *from, const char *to);
	
protected:
	Encoder(const char *from, const char *to);
	virtual ~Encoder();
	
	void check(const IconvHolder &conv) const;
	char* next(const EncoderContext &ctx) const;
	
	virtual size_t rep(char *buf, size_t size, const EncoderContext &ctx) const = 0;
	
private:
	Encoder(const Encoder &);
	Encoder& operator = (const Encoder &);
	friend class std::auto_ptr<Encoder>;
	
private:
	std::string from_;
	IconvHolder iconv_;
};

template<typename Cont> inline std::string 
Encoder::encode(const Cont &cont) const {
	std::string dest;
	encode(createRange(cont), dest);
	return dest;
}
	
template<typename Cont> inline void
Encoder::encode(const Cont &cont, std::string &dest) const {
	encode(createRange(cont), dest);
}

} // namespace xscript

#endif // _XSCRIPT_ENCODER_H_
