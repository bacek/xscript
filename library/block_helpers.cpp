#include "settings.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "internal/block_helpers.h"
#include "xscript/context.h"
#include "xscript/state.h"
#include "xscript/xml_util.h"

namespace xscript {

static const std::string STATE_ARG_PARAM_NAME = "StateArg";

DynamicParam::DynamicParam() : state_(false)
{}

DynamicParam::DynamicParam(const char *value, const char *type) :
    value_(value ? value : ""),
    state_(isStateType(type))
{}

bool
DynamicParam::assign(const char *value, const char *type) {
    value_ = value ? value : "";
    state_ = isStateType(type);
    return NULL == type || state_;
}

std::string
DynamicParam::value(const Context *ctx) const {
    if (!state_) {
        return value_;
    }
    return ctx ? ctx->state()->asString(value_, StringUtils::EMPTY_STRING) :
        StringUtils::EMPTY_STRING;
}

const std::string&
DynamicParam::value() const {
    return value_;
}

bool
DynamicParam::fromState() const {
    return state_;
}

bool
DynamicParam::isStateType(const char *type) const {
    return type && strcasecmp(type, STATE_ARG_PARAM_NAME.c_str()) == 0;
}


XPathExpr::XPathExpr()
{}

XPathExpr::XPathExpr(const char *expression, const char *type) :
    expression_(expression, type)
{}

XPathExpr::~XPathExpr()
{}

bool
XPathExpr::assign(const char *expression, const char *type) {
    return expression_.assign(expression, type);
}

const std::string&
XPathExpr::value() const {
    return expression_.value();
}

bool
XPathExpr::fromState() const {
    return expression_.fromState();
}

std::string
XPathExpr::expression(const Context *ctx) const {
    return expression_.value(ctx);
}

void
XPathExpr::compile() {
    try {
        if (fromState()) {
            return;
        }
        compiled_expr_ = XmlXPathCompExprHelper(xmlXPathCompile(
                (const xmlChar *)value().c_str()));
        XmlUtils::throwUnless(NULL != compiled_expr_.get());
    }
    catch(const std::exception &e) {
        std::string message = "XPath error with expression " + value() + " : ";
        message.append(e.what());
        throw std::runtime_error(message);
    }
}

XmlXPathObjectHelper
XPathExpr::eval(xmlXPathContextPtr context, const Context *ctx) const {
    try {
        XmlXPathObjectHelper xpathObj;
        if (compiled_expr_.get()) {
            xpathObj = XmlXPathObjectHelper(xmlXPathCompiledEval(
                    compiled_expr_.get(), context));
        }
        else {
            xpathObj = XmlXPathObjectHelper(xmlXPathEvalExpression(
                    (const xmlChar *)expression(ctx).c_str(), context));
        }
        XmlUtils::throwUnless(NULL != xpathObj.get());
        return xpathObj;
    }
    catch(const std::exception &e) {
        std::string message = "XPath error with expression " + expression(ctx) + " : ";
        message.append(e.what());
        throw std::runtime_error(message);
    }
}


XPathNodeExpr::XPathNodeExpr(const char *expression, const char *result,
        const char *delimeter, const char *type) :
    expr_(expression, type), result_(result ? result : ""),
    delimeter_(delimeter ? delimeter : "")
{}

XPathNodeExpr::~XPathNodeExpr()
{}

std::string
XPathNodeExpr::expression(const Context *ctx) const {
    return expr_.expression(ctx);
}

const std::string&
XPathNodeExpr::result() const {
    return result_;
}

const std::string&
XPathNodeExpr::delimeter() const {
    return delimeter_;
}

const std::map<std::string, std::string>&
XPathNodeExpr::namespaces() const {
    return namespaces_;
}

void
XPathNodeExpr::addNamespace(const char* prefix, const char* uri) {
    if (prefix && uri) {
        namespaces_.insert(std::make_pair(std::string(prefix), std::string(uri)));
    }
}

void
XPathNodeExpr::compile() {
    expr_.compile();
}

XmlXPathObjectHelper
XPathNodeExpr::eval(xmlXPathContextPtr context, const Context *ctx) const {
    return expr_.eval(context, ctx);
}

Guard::Guard(const char *expr, const char *type, const char *value, bool is_not) :
    guard_(expr ? expr : ""),
    value_(value ? value : ""),
    not_(is_not), state_(false),
    method_(GuardChecker::instance()->method(type ? std::string(type) : STATE_ARG_PARAM_NAME))
{
    if (NULL == method_) {
        std::stringstream stream;
        stream << "Incorrect guard type. Type: ";
        if (type) {
            stream << type;
        }
        stream << ". Guard: " << guard_;
        throw std::runtime_error(stream.str());
    }

    if (!GuardChecker::instance()->allowed(type ? type : STATE_ARG_PARAM_NAME.c_str(), guard_.empty())) {
        std::stringstream stream;
        stream << "Guard is not allowed. Type: ";
        if (type) {
            stream << type;
        }
        stream << ". Guard: " << guard_;
        throw std::runtime_error(stream.str());
    }

    if (NULL == type || 0 == strcasecmp(STATE_ARG_PARAM_NAME.c_str(), type)) {
        state_ = true;
    }
}

bool
Guard::check(const Context *ctx) {
    return not_ ^ (*method_)(ctx, guard_, value_);
}

bool
Guard::isState() const {
    return state_;
}

} // namespace xscript
