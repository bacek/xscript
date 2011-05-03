#ifndef _XSCRIPT_INTERNAL_BLOCK_HELPERS_H_
#define _XSCRIPT_INTERNAL_BLOCK_HELPERS_H_

#include <map>
#include <string>

#include <libxml/xpath.h>

#include "xscript/guard_checker.h"
#include "xscript/xml_helpers.h"

namespace xscript {

class Context;

class DynamicParam {
public:
    DynamicParam();
    DynamicParam(const char *value, const char *type);
    bool assign(const char *value, const char *type);
    std::string value(const Context *ctx) const;
    const std::string& value() const;
    bool fromState() const;
private:
    bool isStateType(const char *type) const;
private:
    std::string value_;
    bool state_;
};

class XPathExpr {
public:
    XPathExpr();
    XPathExpr(const char *expression, const char *type);
    ~XPathExpr();

    bool assign(const char *expression, const char *type);

    const std::string& value() const;
    bool fromState() const;
    std::string expression(const Context *ctx) const;

    void compile();
    bool compiled() const;
    XmlXPathObjectHelper eval(xmlXPathContextPtr context, const Context *ctx) const;

private:
    DynamicParam expression_;
    XmlXPathCompExprHelper compiled_expr_;
};

class XPathNodeExpr {
public:
    XPathNodeExpr(const char *expression, const char *result, const char *delimiter, const char *type);
    ~XPathNodeExpr();

    std::string expression(const Context *ctx) const;
    const std::string& result() const;
    const std::string& delimiter() const;
    const std::map<std::string, std::string>& namespaces() const;
    void addNamespace(const char *prefix, const char *uri);

    void compile();
    XmlXPathObjectHelper eval(xmlXPathContextPtr context, const Context *ctx) const;

private:
    XPathExpr expr_;
    std::string result_;
    std::string delimiter_;
    std::map<std::string, std::string> namespaces_;
};

class Guard {
public:
    Guard(const char *expr, const char *type, const char *value, bool is_not);
    bool check(const Context *ctx);
    bool isState() const;

private:
    std::string guard_;
    std::string value_;
    bool not_;
    bool state_;
    GuardCheckerMethod method_;
};

} // namespace xscript

#endif // _XSCRIPT_INTERNAL_BLOCK_HELPERS_H_
