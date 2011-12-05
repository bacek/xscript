#ifndef _XSCRIPT_EXCEPTION_H_
#define _XSCRIPT_EXCEPTION_H_

#include <stdexcept>
#include <string>
#include <vector>

#include <xscript/xml_helpers.h>

namespace xscript {

class UnboundRuntimeError : public std::exception {
public:
    explicit UnboundRuntimeError(const std::string &error) : error_(error) {}
    virtual ~UnboundRuntimeError() throw () {}
    virtual const char* what() const throw () {
        return error_.c_str();
    }
    const std::string& whatStr() const throw () {
        return error_;
    }
    void append(const std::string &str) {
        error_.append(str);
    }

private:
    std::string error_;
};

class CanNotOpenError : public UnboundRuntimeError {
public:
    explicit CanNotOpenError(const std::string &filename);
    virtual ~CanNotOpenError() throw ();
};

class SkipCacheException : public UnboundRuntimeError {
public:
    explicit SkipCacheException(const std::string &reason);
    virtual ~SkipCacheException() throw ();
};

class BadRequestError : public UnboundRuntimeError {
public:
    explicit BadRequestError(const std::string &error) : UnboundRuntimeError(error) {}
};

class ParseError : public UnboundRuntimeError {
public:
    typedef std::vector<std::string> InfoType;

    explicit ParseError(const std::string &error);
    virtual ~ParseError() throw () {}

    void add(const std::string &info);
    const InfoType& info() const;

private:
    InfoType info_;
};

class InvokeError : public UnboundRuntimeError {
public:
    typedef std::vector<std::pair<std::string, std::string> > InfoMapType;

    explicit InvokeError(const std::string &error);
    InvokeError(const std::string &error, XmlNodeHelper node);
    InvokeError(const std::string &error, const std::string &name, const std::string &value);
    InvokeError(const std::string &error, const InfoMapType &info);

    void add(const std::string &name, const std::string &value);
    void addEscaped(const std::string &name, const std::string &value);
    void attachNode(const XmlNodeHelper &node);

    virtual ~InvokeError() throw () {}
    virtual std::string what_info() const throw();
    virtual const InfoMapType& info() const throw() {
        return info_;
    }
    virtual XmlNodeHelper what_node() const throw() {
        return node_;
    }
private:
    InfoMapType info_;
    XmlNodeHelper node_;
};

class CriticalInvokeError : public InvokeError {
public:
    explicit CriticalInvokeError(const std::string &error) : InvokeError(error) {}
    CriticalInvokeError(const std::string &error, XmlNodeHelper node) :
        InvokeError(error, node) {}
    CriticalInvokeError(const std::string &error, const std::string &name, const std::string &value) :
        InvokeError(error, name, value) {}
};

class SkipResultInvokeError : public InvokeError {
public:
    explicit SkipResultInvokeError(const std::string &error) : InvokeError(error) {}
    SkipResultInvokeError(const std::string &error, const std::string &name, const std::string &value) :
        InvokeError(error, name, value) {}
};

class MetaInvokeError : public InvokeError {
public:
    explicit MetaInvokeError(const std::string &error) : InvokeError(error) {}
    MetaInvokeError(const InvokeError &error) : InvokeError(error.what(), error.info()) {}
};

} // namespace xscript

#endif // _XSCRIPT_EXCEPTION_H_
