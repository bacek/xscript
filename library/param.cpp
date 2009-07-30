#include "settings.h"

#include <cassert>
#include <sstream>
#include <stdexcept>
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/current_function.hpp>

#include "xscript/args.h"
#include "xscript/xml_util.h"
#include "xscript/param.h"
#include "xscript/logger.h"
#include "xscript/validator_factory.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

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

    static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
};

Param::Param(Object * /* owner */, xmlNodePtr node) :
        node_(node) {
    assert(node_);
}

Param::~Param() {
}

bool
Param::constant() const {
    return false;
}

void
Param::parse() {
    // Create validator if any. Validators will remove used attributes.
    validator_ = ValidatorFactory::instance()->createValidator(node_);

    XmlUtils::visitAttributes(node_->properties,
                              boost::bind(&Param::property, this, _1, _2));

    xmlNodePtr c = node_->children;
    if (c && xmlNodeIsText(c) && c->content) {
        value_.assign((const char*) c->content);
    }
}

const std::string&
Param::value() const {
    return value_;
}

void
Param::property(const char *name, const char *value) {
    if (strncasecmp(name, "id", sizeof("id")) == 0) {
        id_.assign(value);
    }
    else if (strncasecmp(name, "type", sizeof("type")) != 0) {
        std::stringstream stream;
        stream << "incorrect param attribute: " << name;
        throw std::invalid_argument(stream.str());
    }
}

void
Param::checkValidator(const Context *ctx) const {
    if (validator_.get()) {
        validator_->check(ctx, *this);
    }
};

template<typename T>
SimpleParam<T>::SimpleParam(Object *owner, xmlNodePtr node) :
        Param(owner, node) {
}

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
SimpleParam<std::string>::type() const {
    return "string";
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
    try {
        return boost::lexical_cast<T>(value());
    }
    catch (const std::exception &e) {
        log()->error("caught exception while getting value: %s", e.what());
        throw;
    }
}

template<> bool
SimpleParam<bool>::typedValue() const {
    try {
        if ("0" == value() || strncasecmp(value().c_str(), "false", sizeof("false")) == 0) {
            return false;
        }
        else if ("1" == value() || strncasecmp(value().c_str(), "true", sizeof("true")) == 0) {
            return true;
        }
        else {
            throw std::invalid_argument(std::string("bad boolean value: ").append(value()));
        }
    }
    catch (const std::exception &e) {
        log()->error("caught exception while getting value: %s", e.what());
        throw;
    }
}

template<> std::string
SimpleParam<std::string>::typedValue() const {
    return value();
}

template<typename T> std::string
SimpleParam<T>::asString(const Context * /* ctx */) const {
    return value();
}

template<typename T> void
SimpleParam<T>::add(const Context * /* ctx */, ArgList &al) const {
    al.add(typedValue());
}

template<typename T> std::auto_ptr<Param>
SimpleParam<T>::create(Object *owner, xmlNodePtr node) {
    return std::auto_ptr<Param>(new SimpleParam<T>(owner, node));
}


ConvertedParam::ConvertedParam(Object *owner, xmlNodePtr node) :
        Param(owner, node) {

}

ConvertedParam::~ConvertedParam() {
}


void
ConvertedParam::add(const Context *ctx, ArgList &al) const {
    al.addAs(as(), asString(ctx));
}

void
ConvertedParam::property(const char *name, const char *value) {
    if (strncasecmp(name, "as", sizeof("as")) == 0) {
        as_.assign(value);
    }
    else Param::property(name, value);
}


TypedParam::TypedParam(Object *owner, xmlNodePtr node) :
        ConvertedParam(owner, node) {

}

TypedParam::~TypedParam() {
}

void
TypedParam::property(const char *name, const char *value) {
    if (strncasecmp(name, "default", sizeof("default")) == 0) {
        default_value_.assign(value);
    }
    else ConvertedParam::property(name, value);
}

const std::string&
TypedParam::value() const {
    const std::string &v = ConvertedParam::value();
    if (!v.empty()) {
        return v;
    }
    return id();
}

void
TypedParam::add(const Context *ctx, ArgList &al) const {
    const std::string& as = ConvertedParam::as();
    if (as.empty()) {
        al.add(asString(ctx));
    }
    else {
        al.addAs(as, asString(ctx));
    }
}

class SimpleParamRegisterer {
public:
    SimpleParamRegisterer() {

        CreatorRegisterer("bool", &SimpleParam<bool>::create);
        CreatorRegisterer("boolean", &SimpleParam<bool>::create);

        CreatorRegisterer("float", &SimpleParam<float>::create);
        CreatorRegisterer("double", &SimpleParam<double>::create);
        CreatorRegisterer("string", &SimpleParam<std::string>::create);

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
