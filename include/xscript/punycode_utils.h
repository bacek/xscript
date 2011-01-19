#ifndef _XSCRIPT_PUNYCODE_UTILS_H_
#define _XSCRIPT_PUNYCODE_UTILS_H_

#include <string>
#include <memory>

#include <xscript/range.h>


namespace xscript {

class Encoder;

class PunycodeEncoder {
public:
    PunycodeEncoder(); // from utf-8
    explicit PunycodeEncoder(const char *from);

    virtual ~PunycodeEncoder();


    void encode(const Range &val, std::string &dest) const;
    std::string encode(const Range &val) const;

    template<typename Cont> std::string encode(const Cont &cont) const;
    template<typename Cont> void encode(const Cont &cont, std::string &dest) const;


    void domainEncode(const Range &val, std::string &dest) const;
    std::string domainEncode(const Range &val) const;

    template<typename Cont> std::string domainEncode(const Cont &cont) const;
    template<typename Cont> void domainEncode(const Cont &cont, std::string &dest) const;

private:
    PunycodeEncoder(const PunycodeEncoder &);
    PunycodeEncoder& operator = (const PunycodeEncoder &);

    std::auto_ptr<Encoder> to_ucs4_;
};


class PunycodeDecoder {
public:
    PunycodeDecoder(); // to utf-8
    explicit PunycodeDecoder(const char *to);

    virtual ~PunycodeDecoder();


    void decode(const Range &val, std::string &dest) const;
    std::string decode(const Range &val) const;

    template<typename Cont> std::string decode(const Cont &cont) const;
    template<typename Cont> void decode(const Cont &cont, std::string &dest) const;


    void domainDecode(const Range &val, std::string &dest) const;
    std::string domainDecode(const Range &val) const;

    template<typename Cont> std::string domainDecode(const Cont &cont) const;
    template<typename Cont> void domainDecode(const Cont &cont, std::string &dest) const;

private:
    PunycodeDecoder(const PunycodeDecoder &);
    PunycodeDecoder& operator = (const PunycodeDecoder &);

    std::auto_ptr<Encoder> from_ucs4_;
};


inline std::string
PunycodeEncoder::encode(const Range &val) const {
    std::string dest;
    encode(val, dest);
    return dest;
}

template<typename Cont> inline std::string
PunycodeEncoder::encode(const Cont &cont) const {
    std::string dest;
    encode(createRange(cont), dest);
    return dest;
}

template<typename Cont> inline void
PunycodeEncoder::encode(const Cont &cont, std::string &dest) const {
    encode(createRange(cont), dest);
}

inline std::string
PunycodeEncoder::domainEncode(const Range &val) const {
    std::string dest;
    domainEncode(val, dest);
    return dest;
}

template<typename Cont> inline std::string
PunycodeEncoder::domainEncode(const Cont &cont) const {
    std::string dest;
    domainEncode(createRange(cont), dest);
    return dest;
}

template<typename Cont> inline void
PunycodeEncoder::domainEncode(const Cont &cont, std::string &dest) const {
    domainEncode(createRange(cont), dest);
}


inline std::string
PunycodeDecoder::decode(const Range &val) const {
    std::string dest;
    decode(val, dest);
    return dest;
}

template<typename Cont> inline std::string
PunycodeDecoder::decode(const Cont &cont) const {
    std::string dest;
    decode(createRange(cont), dest);
    return dest;
}

template<typename Cont> inline void
PunycodeDecoder::decode(const Cont &cont, std::string &dest) const {
    decode(createRange(cont), dest);
}

inline std::string
PunycodeDecoder::domainDecode(const Range &val) const {
    std::string dest;
    domainDecode(val, dest);
    return dest;
}

template<typename Cont> inline std::string
PunycodeDecoder::domainDecode(const Cont &cont) const {
    std::string dest;
    domainDecode(createRange(cont), dest);
    return dest;
}

template<typename Cont> inline void
PunycodeDecoder::domainDecode(const Cont &cont, std::string &dest) const {
    domainDecode(createRange(cont), dest);
}

};

#endif // _XSCRIPT_PUNYCODE_UTILS_H_
