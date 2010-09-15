#include "settings.h"

#include <stdexcept>

#include <string.h>

#include "xscript/args.h"
#include "xscript/param.h"

#include <boost/lexical_cast.hpp>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class StringParam : public Param {
public:
    StringParam(Object *owner, xmlNodePtr node);
    virtual ~StringParam();

    virtual const char* type() const;
    virtual bool constant() const;
    virtual std::string asString(const Context *ctx) const;
    virtual void add(const Context *ctx, ArgList &al) const;

    static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
};

template<typename T>
class SimpleParam : public Param {
public:
    SimpleParam(Object *owner, xmlNodePtr node);
    virtual ~SimpleParam();

    virtual const char* type() const;
    virtual bool constant() const;
    T typedValue() const;
    virtual std::string asString(const Context *ctx) const;
    virtual void add(const Context *ctx, ArgList &al) const;
    virtual void parse();

    static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
private:
    T value_;
};

StringParam::StringParam(Object *owner, xmlNodePtr node) : Param(owner, node)
{}

StringParam::~StringParam() {
}

bool
StringParam::constant() const {
    return true;
}

const char*
StringParam::type() const {
    return "string";
}

std::string
StringParam::asString(const Context * /* ctx */) const {
    return value();
}

void
StringParam::add(const Context * /* ctx */, ArgList &al) const {
    al.add(value());
}

std::auto_ptr<Param>
StringParam::create(Object *owner, xmlNodePtr node) {
    return std::auto_ptr<Param>(new StringParam(owner, node));
}


template<typename T>
SimpleParam<T>::SimpleParam(Object *owner, xmlNodePtr node) :
    Param(owner, node)
{}

template<typename T>
SimpleParam<T>::~SimpleParam() {
}

template<typename T> bool
SimpleParam<T>::constant() const {
    return true;
}

template<typename T> const char*
SimpleParam<T>::type() const {
    throw std::logic_error("Undefined simple param type");
}

template<> const char*
SimpleParam<bool>::type() const {
    return "boolean";
}

template<> const char*
SimpleParam<float>::type() const {
    return "float";
}

template<> const char*
SimpleParam<double>::type() const {
    return "double";
}

template<> const char*
SimpleParam<boost::int32_t>::type() const {
    return "long";
}

template<> const char*
SimpleParam<boost::uint32_t>::type() const {
    return "unsigned long";
}

template<> const char*
SimpleParam<boost::int64_t>::type() const {
    return "long long";
}

template<> const char*
SimpleParam<boost::uint64_t>::type() const {
    return "unsigned long long";
}

template<typename T> T
SimpleParam<T>::typedValue() const {
    return value_;
}

template<typename T> std::string
SimpleParam<T>::asString(const Context * /* ctx */) const {
    return value();
}

template<typename T> void
SimpleParam<T>::add(const Context * /* ctx */, ArgList &al) const {
    CommonArgList* args = dynamic_cast<CommonArgList*>(&al);
    args ? args->add(value()) : al.add(typedValue());
}

template<typename T> void
SimpleParam<T>::parse() {
    Param::parse();
    try {
        value_ = boost::lexical_cast<T>(value());
    }
    catch (const std::exception &e) {
        throw std::invalid_argument(std::string("bad value: ").append(value()));
    }
}

template<> void
SimpleParam<bool>::parse() {
    Param::parse();
    if ("0" == value() || strncasecmp(value().c_str(), "false", sizeof("false")) == 0) {
        value_ = false;
    }
    else if ("1" == value() || strncasecmp(value().c_str(), "true", sizeof("true")) == 0) {
        value_ = true;
    }
    else {
        throw std::invalid_argument(std::string("bad boolean value: ").append(value()));
    }
}

template<typename T> std::auto_ptr<Param>
SimpleParam<T>::create(Object *owner, xmlNodePtr node) {
    return std::auto_ptr<Param>(new SimpleParam<T>(owner, node));
}

class SimpleParamRegisterer {
public:
    SimpleParamRegisterer() {

        CreatorRegisterer("bool", &SimpleParam<bool>::create);
        CreatorRegisterer("boolean", &SimpleParam<bool>::create);

        CreatorRegisterer("float", &SimpleParam<float>::create);
        CreatorRegisterer("double", &SimpleParam<double>::create);
        CreatorRegisterer("string", &StringParam::create);

        CreatorRegisterer("long", &SimpleParam<boost::int32_t>::create);

        CreatorRegisterer("ulong", &SimpleParam<boost::uint32_t>::create);
        CreatorRegisterer("unsigned long", &SimpleParam<boost::uint32_t>::create);

        CreatorRegisterer("longlong", &SimpleParam<boost::int64_t>::create);
        CreatorRegisterer("long long", &SimpleParam<boost::int64_t>::create);

        CreatorRegisterer("ulonglong", &SimpleParam<boost::uint64_t>::create);
        CreatorRegisterer("unsigned long long", &SimpleParam<boost::uint64_t>::create);
    }
};

static SimpleParamRegisterer reg_;

} // namespace xscript
