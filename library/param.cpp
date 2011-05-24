#include "settings.h"

#include <cassert>

#include <boost/bind.hpp>

#include "xscript/args.h"
#include "xscript/param.h"
#include "xscript/validator.h"
#include "xscript/validator_factory.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class Param::ParamData {
public:
    ParamData(xmlNodePtr node) : node_(node) {
        assert(node_);
    }
    ~ParamData() {}

    void parse() {
        // Create validator if any. Validators will remove used attributes.
        validator_ = ValidatorFactory::instance()->createValidator(node_);

        const char *content = XmlUtils::value(node_);
        if (NULL != content) {
            value_.assign(content);
        }
    }

    xmlNodePtr node_;
    std::string id_, value_;
    std::auto_ptr<Validator> validator_;
};

Param::Param(Object * /* owner */, xmlNodePtr node) :
    data_(new ParamData(node)) {
}

Param::~Param() {
}

const std::string&
Param::id() const {
    return data_->id_;
}

bool
Param::constant() const {
    return false;
}

void
Param::parse() {
    data_->parse();
}

void
Param::visitProperties() {
    XmlUtils::visitAttributes(data_->node_->properties,
                              boost::bind(&Param::property, this, _1, _2));
}

const std::string&
Param::value() const {
    return data_->value_;
}

void
Param::property(const char *name, const char *value) {
    if (!strncasecmp(name, "id", sizeof("id"))) {
        data_->id_.assign(value);
    }
    else if (strncasecmp(name, "type", sizeof("type")) != 0) {
        std::stringstream stream;
        stream << "incorrect param attribute: " << name;
        throw std::invalid_argument(stream.str());
    }
}

void
Param::checkValidator(const Context *ctx) const {
    if (data_->validator_.get()) {
        data_->validator_->check(ctx, *this);
    }
};


class ConvertedParam::ConvertedParamData {
public:
    ConvertedParamData() {}
    ~ConvertedParamData() {}
    
    std::string as_;
};

ConvertedParam::ConvertedParam(Object *owner, xmlNodePtr node) :
        Param(owner, node), converted_data_(new ConvertedParamData()) {
}

ConvertedParam::~ConvertedParam() {
}

const std::string&
ConvertedParam::as() const {
    return converted_data_->as_;
}

void
ConvertedParam::add(const Context *ctx, ArgList &al) const {
    al.addAs(as(), asString(ctx));
}

void
ConvertedParam::property(const char *name, const char *value) {
    if (strncasecmp(name, "as", sizeof("as")) == 0) {
        converted_data_->as_.assign(value);
    }
    else Param::property(name, value);
}

class TypedParam::TypedParamData {
public:
    TypedParamData() {}
    ~TypedParamData() {}
    
    std::string default_value_;
};

TypedParam::TypedParam(Object *owner, xmlNodePtr node) :
        ConvertedParam(owner, node), typed_data_(new TypedParamData()) {
}

TypedParam::~TypedParam() {
}

const std::string&
TypedParam::defaultValue() const {
    return typed_data_->default_value_;
}

void
TypedParam::property(const char *name, const char *value) {
    if (strncasecmp(name, "default", sizeof("default")) == 0) {
        typed_data_->default_value_.assign(value);
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

TypedParam::ValueResult
TypedParam::getValue(const Context *ctx) const {
    (void)ctx;
    return ValueResult(StringUtils::EMPTY_STRING, false);
}

void
TypedParam::add(const Context *ctx, ArgList &al) const {
    const std::string& as = ConvertedParam::as();
    if (as.empty()) {
        al.add(asString(ctx));
    }
    else if (NULL == dynamic_cast<CommonArgList*>(&al)) {
        ConvertedParam::add(ctx, al);
    }
    else {
        ValueResult result = getValue(ctx);
        al.add(result.second ? result.first : defaultValue());
    }
}

std::string
TypedParam::asString(const Context *ctx) const {
    ValueResult result = getValue(ctx);
    return result.second ? result.first : defaultValue();
}

} // namespace xscript
