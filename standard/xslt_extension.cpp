#include <fstream>

#include <xscript/context.h>
#include <xscript/logger.h>
#include <xscript/script.h>
#include <xscript/stylesheet.h>
#include <xscript/xml_util.h>
#include <xscript/xslt_extension.h>

#include <libxml/xpathInternals.h>
#include <libxslt/xsltutils.h>
#include <libxslt/extensions.h>

#include <glib.h>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

class StandardXsltExtensions
{
public:
    StandardXsltExtensions();
};

static StandardXsltExtensions standardXsltExtensions;

extern "C" void
xscriptXsltToLower(xmlXPathParserContextPtr ctxt, int nargs)
{
    log()->entering("xscript:tolower");
    if (NULL == ctxt) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (1 != nargs) {
        XmlUtils::reportXsltError("xscript:tolower: bad param count", ctxt);
        return;
    }

    const char* str = params.str(0);

    if (NULL == str) {
        XmlUtils::reportXsltError("xscript:tolower: bad parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }
    
    try {
        gchar* ret = g_utf8_strdown(str, strlen(str));
        valuePush(ctxt, xmlXPathNewCString((char*)ret));
        g_free(ret);
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:tolower: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:tolower: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
    }
}

extern "C" void
xscriptXsltToUpper(xmlXPathParserContextPtr ctxt, int nargs)
{
    log()->entering("xscript:toupper");
    if (NULL == ctxt) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (1 != nargs) {
        XmlUtils::reportXsltError("xscript:toupper: bad param count", ctxt);
        return;
    }

    const char* str = params.str(0);

    if (NULL == str) {
        XmlUtils::reportXsltError("xscript:toupper: bad parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    try {
        gchar* ret = g_utf8_strup(str, strlen(str));
        valuePush(ctxt, xmlXPathNewCString((char*)ret));
        g_free(ret);
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:toupper: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:toupper: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
    }
}


StandardXsltExtensions::StandardXsltExtensions() {
    XsltFunctionRegisterer("tolower", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltToLower);
    XsltFunctionRegisterer("toupper", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltToUpper);
}

} // namespace xscript
