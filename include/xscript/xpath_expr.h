#ifndef _XSCRIPT_XPATH_EXPR_H_
#define _XSCRIPT_XPATH_EXPR_H_

#include <string>

namespace xscript
{

/**
 * Storage for XPath expressions executed for evaluated block.
 */
class XPathExpr {
public:
    XPathExpr(const char* expression, const char* result, const char* delimeter) :
            expression_(expression ? expression : ""),
            result_(result ? result : ""),
            delimeter_(delimeter ? delimeter : "") {}
    ~XPathExpr() {}

    typedef std::list<std::pair<std::string, std::string> > NamespaceListType;

    const std::string& expression() const {
        return expression_;
    }

    const std::string& result() const {
        return result_;
    }

    const std::string& delimeter() const {
        return delimeter_;
    }

    const NamespaceListType& namespaces() const {
        return namespaces_;
    }

    void addNamespace(const char* prefix, const char* uri) {
        if (prefix && uri) {
            namespaces_.push_back(std::make_pair(std::string(prefix), std::string(uri)));
        }
    }

private:
    std::string expression_;
    std::string result_;
    std::string delimeter_;
    NamespaceListType namespaces_;
};

}

#endif 
