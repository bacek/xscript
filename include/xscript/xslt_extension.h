#ifndef _XSCRIPT_XSLT_EXTENSION_H_
#define _XSCRIPT_XSLT_EXTENSION_H_

#include <vector>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxslt/xsltInternals.h>
#include <boost/utility.hpp>

namespace xscript {

class XsltFunctionRegisterer : private boost::noncopyable {
public:
    XsltFunctionRegisterer(const char* name, const char* nsref, xmlXPathFunction function);
};

class XsltElementRegisterer : private boost::noncopyable {
public:
    XsltElementRegisterer(const char* name, const char* nsref, xsltTransformFunction func);
    static void registerBlockInvokation(const char *name, const char *nsref);
};

class XsltParamFetcher : private boost::noncopyable {
public:
    XsltParamFetcher(xmlXPathParserContextPtr ctxt, int nargs);
    virtual ~XsltParamFetcher();

    void clear();
    unsigned int size() const;
    const char* str(unsigned int index) const;

private:
    std::vector<xmlChar*> strings_;
};

} // namespace xscript

#endif // _XSCRIPT_XSLT_EXTENSION_H_
