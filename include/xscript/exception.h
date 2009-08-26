#ifndef _XSCRIPT_EXCEPTION_H_
#define _XSCRIPT_EXCEPTION_H_

#include <stdexcept>
#include <string>
#include <vector>

#include <xscript/xml_helpers.h>

namespace xscript {

class UnboundRuntimeError : public std::exception {
public:
    UnboundRuntimeError(const std::string &error) : error_(error) {}
    virtual ~UnboundRuntimeError() throw () {}
    virtual const char* what() const throw () {
        return error_.c_str();
    }

private:
    std::string error_;
};

class BadRequestError : public UnboundRuntimeError {
public:
    BadRequestError(const std::string &error) : UnboundRuntimeError(error) {}
};


class InvokeError : public UnboundRuntimeError {
public:
    typedef std::vector<std::pair<std::string, std::string> > InfoMapType;

    InvokeError(const std::string &error);
    InvokeError(const std::string &error, XmlNodeHelper node);
    InvokeError(const std::string &error, const std::string &name, const std::string &value);

    void add(const std::string &name, const std::string &value);
    void addEscaped(const std::string &name, const std::string &value);

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
    CriticalInvokeError(const std::string &error) : InvokeError(error) {}
    CriticalInvokeError(const std::string &error, XmlNodeHelper node) :
        InvokeError(error, node) {}
    CriticalInvokeError(const std::string &error, const std::string &name, const std::string &value) :
        InvokeError(error, name, value) {}
};

class SkipResultInvokeError : public InvokeError {
public:
    SkipResultInvokeError(const std::string &error) : InvokeError(error) {}
    SkipResultInvokeError(const std::string &error, const std::string &name, const std::string &value) :
        InvokeError(error, name, value) {}
};

} // namespace xscript

#endif // _XSCRIPT_EXCEPTION_H_
