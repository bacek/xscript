#include "settings.h"

#include <sstream>
#include <algorithm>
#include <exception>
#include <boost/lexical_cast.hpp>
#include <boost/current_function.hpp>

#include <libxslt/xsltutils.h>
#include <libxslt/transform.h>
#include <libxslt/extensions.h>

#include <libxml/xpathInternals.h>

#include "xscript/block.h"
#include "xscript/context.h"
#include "xscript/encoder.h"
#include "xscript/logger.h"
#include "xscript/protocol.h"
#include "xscript/range.h"
#include "xscript/response.h"
#include "xscript/script.h"
#include "xscript/state.h"
#include "xscript/stylesheet.h"
#include "xscript/util.h"
#include "xscript/xml_util.h"
#include "xscript/xml_helpers.h"
#include "xscript/xslt_extension.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class XsltExtensions {
public:
    XsltExtensions();
};

static XsltExtensions xsltExtensions;

extern "C" void
xscriptXsltHttpHeaderOut(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:http-header-out");
    if (NULL == ctxt) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (2 != nargs) {
        XmlUtils::reportXsltError("xscript:http-header-out: bad param count", ctxt);
        return;
    }

    const char* name = params.str(0);
    const char* value = params.str(1);

    if (NULL == name || NULL == value) {
        XmlUtils::reportXsltError("xscript:http-header-out: bad parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    xsltTransformContextPtr tctx = xsltXPathGetTransformContext(ctxt);
    if (NULL == tctx) {
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    Context *ctx = NULL;
    try {
        ctx = Stylesheet::getContext(tctx);
        Response *response = ctx->response();
        response->setHeader(name, value);
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:http-header-out: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:http-header-out: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
    }
    xmlXPathReturnEmptyNodeSet(ctxt);
}

extern "C" void
xscriptXsltHttpRedirect(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:http-redirect");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (1 != nargs) {
        XmlUtils::reportXsltError("xscript:http-redirect: bad param count", ctxt);
        return;
    }

    const char* str = params.str(0);
    if (NULL == str) {
        XmlUtils::reportXsltError("xscript:http-redirect: bad parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    xsltTransformContextPtr tctx = xsltXPathGetTransformContext(ctxt);
    if (NULL == tctx) {
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    Context *ctx = NULL;
    try {
        ctx = Stylesheet::getContext(tctx);
        Response* response = ctx->response();
        response->redirectToPath(str);
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:http-redirect: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:http-redirect: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
    }
    xmlXPathReturnEmptyNodeSet(ctxt);
}

extern "C" void
xscriptXsltSetHttpStatus(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:set-http-status");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (1 != nargs) {
        XmlUtils::reportXsltError("xscript:set-http-status: bad param count", ctxt);
        return;
    }

    const char* str = params.str(0);
    if (NULL == str) {
        XmlUtils::reportXsltError("xscript:set-http-status: bad parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    xsltTransformContextPtr tctx = xsltXPathGetTransformContext(ctxt);
    if (NULL == tctx) {
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    Context* ctx = NULL;
    try {
        unsigned short status = boost::lexical_cast<unsigned short>(str);
        ctx = Stylesheet::getContext(tctx);
        Response *response = ctx->response();
        response->setStatus(status);
        xmlXPathReturnNumber(ctxt, status);
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:set-http-status: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:set-http-status: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
}

extern "C" void
xscriptXsltGetStateArg(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:get-state-arg");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (nargs < 1 || nargs > 2) {
        XmlUtils::reportXsltError("xscript:get-state-arg: bad param count", ctxt);
        return;
    }

    const char* str = params.str(0);
    if (NULL == str) {
        XmlUtils::reportXsltError("xscript:get-state-arg: bad first parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    const char* default_value = "";
    if (nargs == 2) {
        default_value = params.str(1);
        if (NULL == default_value) {
            XmlUtils::reportXsltError("xscript:get-state-arg: bad second parameter", ctxt);
            xmlXPathReturnEmptyNodeSet(ctxt);
            return;
        }
    }

    xsltTransformContextPtr tctx = xsltXPathGetTransformContext(ctxt);
    if (NULL == tctx) {
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    Context *ctx = NULL;
    try {
        ctx = Stylesheet::getContext(tctx);
        State* state = ctx->state();
        std::string name(str);
        if (state->has(name)) {
            valuePush(ctxt, xmlXPathNewCString(state->asString(name).c_str()));
        }
        else {
            valuePush(ctxt, xmlXPathNewCString(default_value));
        }
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:get-state-arg: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:get-state-arg: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
}


extern "C" void
xscriptXsltGetProtocolArg(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:get-protocol-arg");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (1 != nargs) {
        XmlUtils::reportXsltError("xscript:get-protocol-arg: bad param count", ctxt);
        return;
    }

    const char* str = params.str(0);
    if (NULL == str) {
        XmlUtils::reportXsltError("xscript:get-protocol-arg: bad parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    if (*str == '\0') {
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    xsltTransformContextPtr tctx = xsltXPathGetTransformContext(ctxt);
    if (NULL == tctx) {
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    const Context *ctx = NULL;
    try {
        ctx = Stylesheet::getContext(tctx);
        std::string value = Protocol::get(ctx, str);
        if (!value.empty()) {
            valuePush(ctxt, xmlXPathNewCString(value.c_str()));
        }
        else {
            xmlXPathReturnEmptyNodeSet(ctxt);
        }
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:get-protocol-arg: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:get-protocol-arg: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
}

extern "C" void
xscriptXsltGetQueryArg(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:get-query-arg");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (nargs < 1 || nargs > 2) {
        XmlUtils::reportXsltError("xscript:get-query-arg: bad param count", ctxt);
        return;
    }

    const char* str = params.str(0);
    if (NULL == str) {
        XmlUtils::reportXsltError("xscript:get-query-arg: bad first parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    const char* default_value = "";
    if (nargs == 2) {
        default_value = params.str(1);
        if (NULL == default_value) {
            XmlUtils::reportXsltError("xscript:get-query-arg: bad second parameter", ctxt);
            xmlXPathReturnEmptyNodeSet(ctxt);
            return;
        }
    }

    xsltTransformContextPtr tctx = xsltXPathGetTransformContext(ctxt);
    if (NULL == tctx) {
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    Context *ctx = NULL;
    try {
        ctx = Stylesheet::getContext(tctx);
        std::string name(str);
        if (ctx->request()->hasArg(name)) {
            valuePush(ctxt, xmlXPathNewCString(ctx->request()->getArg(name).c_str()));
        }
        else {
            valuePush(ctxt, xmlXPathNewCString(default_value));
        }
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:get-query-arg: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:get-query-arg: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
}

extern "C" void
xscriptXsltGetHeader(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:get-header");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (nargs < 1 || nargs > 2) {
        XmlUtils::reportXsltError("xscript:get-header: bad param count", ctxt);
        return;
    }

    const char* str = params.str(0);
    if (NULL == str) {
        XmlUtils::reportXsltError("xscript:get-header: bad parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    const char* default_value = "";
    if (nargs == 2) {
        default_value = params.str(1);
        if (NULL == default_value) {
            XmlUtils::reportXsltError("xscript:get-header: bad second parameter", ctxt);
            xmlXPathReturnEmptyNodeSet(ctxt);
            return;
        }
    }

    xsltTransformContextPtr tctx = xsltXPathGetTransformContext(ctxt);
    if (NULL == tctx) {
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    Context *ctx = NULL;
    try {
        ctx = Stylesheet::getContext(tctx);
        std::string name(str);
        if (ctx->request()->hasHeader(name)) {
            valuePush(ctxt, xmlXPathNewCString(ctx->request()->getHeader(name).c_str()));
        }
        else {
            valuePush(ctxt, xmlXPathNewCString(default_value));
        }
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:get-header: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:get-header: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
}

extern "C" void
xscriptXsltGetCookie(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:get-cookie");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (nargs < 1 || nargs > 2) {
        XmlUtils::reportXsltError("xscript:get-cookie: bad param count", ctxt);
        return;
    }

    const char* str = params.str(0);
    if (NULL == str) {
        XmlUtils::reportXsltError("xscript:get-cookie: bad parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    const char* default_value = "";
    if (nargs == 2) {
        default_value = params.str(1);
        if (NULL == default_value) {
            XmlUtils::reportXsltError("xscript:get-header: bad second parameter", ctxt);
            xmlXPathReturnEmptyNodeSet(ctxt);
            return;
        }
    }

    xsltTransformContextPtr tctx = xsltXPathGetTransformContext(ctxt);
    if (NULL == tctx) {
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    Context *ctx = NULL;
    try {
        ctx = Stylesheet::getContext(tctx);
        std::string name(str);
        if (ctx->request()->hasCookie(name)) {
            valuePush(ctxt, xmlXPathNewCString(ctx->request()->getCookie(name).c_str()));
        }
        else {
            valuePush(ctxt, xmlXPathNewCString(default_value));
        }
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:get-cookie: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:get-cookie: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
}

extern "C" void
xscriptXsltSanitize(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:sanitize");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (nargs < 1 || nargs > 3) {
        XmlUtils::reportXsltError("xscript:sanitize: bad param count", ctxt);
        return;
    }

    const char* str = params.str(0);
    if (NULL == str) {
        XmlUtils::reportXsltError("xscript:sanitize: bad parameter content", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    const char *base_url = NULL;
    if (nargs > 1) {
        base_url = params.str(1);
        if (NULL == base_url) {
            XmlUtils::reportXsltError("xscript:sanitize: bad parameter base url", ctxt);
            xmlXPathReturnEmptyNodeSet(ctxt);
            return;
        }
    }

    Context* ctx = NULL;
    try {
        unsigned int line_limit = 0;
        if (nargs > 2) {
            const char* limit_str = params.str(2);
            if (NULL == limit_str) {
                XmlUtils::reportXsltError("xscript:sanitize: bad parameter line limit", ctxt);
                xmlXPathReturnEmptyNodeSet(ctxt);
                return;
            }
            try {
                line_limit = boost::lexical_cast<unsigned int>(limit_str);
            }
            catch(const boost::bad_lexical_cast &e) {
                XmlUtils::reportXsltError("xscript:sanitize: bad format of line limit parameter", ctxt);
                xmlXPathReturnEmptyNodeSet(ctxt);
                return;
            }
        }

        xsltTransformContextPtr tctx = xsltXPathGetTransformContext(ctxt);
        if (NULL == tctx) {
            xmlXPathReturnEmptyNodeSet(ctxt);
            return;
        }

        std::stringstream stream;
        stream << "<sanitized>" <<
            XmlUtils::sanitize(str, base_url ? std::string(base_url) : StringUtils::EMPTY_STRING, line_limit) <<
                "</sanitized>" << std::endl;
        std::string str = stream.str();

        XmlDocHelper doc(xmlParseMemory(str.c_str(), str.length()));
        XmlUtils::throwUnless(NULL != doc.get());

        xmlNodePtr node = xmlCopyNode(xmlDocGetRootElement(doc.get()), 1);

        ctx = Stylesheet::getContext(tctx);
        ctx->addNode(node);

        xmlNodeSetPtr ret = xmlXPathNodeSetCreate(node);
        xmlXPathReturnNodeSet(ctxt, ret);
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:sanitize: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:sanitize: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
}

extern "C" void
xscriptXsltUrlencode(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:urlencode");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (nargs < 1 || nargs > 2) {
        XmlUtils::reportXsltError("xscript:urlencode: bad param count", ctxt);
        return;
    }

    const char* encoding = NULL;
    const char* value = params.str(0);
    if (nargs == 2) {
        encoding = value;
        value = params.str(1);
    }

    if (NULL == value || (2 == nargs && NULL == encoding)) {
        XmlUtils::reportXsltError("xscript:urlencode: bad parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    try {
        std::string encoded;
        if (NULL != encoding) {
            std::auto_ptr<Encoder> encoder = Encoder::createEscaping("utf-8", encoding);
            encoder->encode(createRange(value), encoded);
        }
        else {
            encoded = value;
        }
        valuePush(ctxt, xmlXPathNewCString(StringUtils::urlencode(encoded).c_str()));
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:urlencode: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:urlencode: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
}

extern "C" void
xscriptXsltUrldecode(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:urldecode");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);


    if (nargs < 1 || nargs > 2) {
        XmlUtils::reportXsltError("xscript:urldecode: bad param count", ctxt);
        return;
    }

    const char* encoding = NULL;
    const char* value = params.str(0);
    if (nargs == 2) {
        encoding = value;
        value = params.str(1);
    }

    if (NULL == value || (2 == nargs && NULL == encoding)) {
        XmlUtils::reportXsltError("xscript:urldecode: bad parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    try {
        std::string decoded;
        if (NULL != encoding) {
            std::auto_ptr<Encoder> encoder = Encoder::createEscaping(encoding, "utf-8");
            encoder->encode(StringUtils::urldecode(value), decoded);
        }
        else {
            decoded = StringUtils::urldecode(value);
        }
        valuePush(ctxt, xmlXPathNewCString(decoded.c_str()));
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:urldecode: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:urldecode: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
}


void
xscriptTemplatedEscStr(const char *str, const char *esc_template, std::string &result) {

    std::string s(str);
    std::string::size_type pos = (std::string::size_type)-1;
    while (true) {
        pos = s.find("\r\n", pos + 1);
        if (pos == std::string::npos) {
            break;
        }
        s.erase(pos, 1);
    }

    for (std::string::const_iterator i = s.begin(), end = s.end(); i != end; ++i) {
        char ch = *i;
        if (ch == '\r' || ch == '\n') {
            result.push_back('\\');
            ch = 'n';
        }
        else {
            const char *res = strchr(esc_template, ch);
            if (NULL != res) {
                result.push_back('\\');
            }
        }
        result.push_back(ch);
    }
}

void
xscriptXsltJSONQuoteStr(const char *str, std::string &result) {

    const char* end = str + strlen(str);
    
    while (str < end) {
        char ch = *str;
        if (ch == '"' || ch == '/') {
            result.push_back('\\');
        }
		else if (ch == '\b') {
			result.push_back('\\');
			ch = 'b';
		}
		else if (ch == '\f') {
			result.push_back('\\');
			ch = 'f';
		}
		else if (ch == '\n') {
			result.push_back('\\');
			ch = 'n';
		}
		else if (ch == '\r') {
			result.push_back('\\');
			ch = 'r';
		}
		else if (ch == '\t') {
			result.push_back('\\');
			ch = 't';
		}
        else if (ch == '\\' && end - str > 5 &&
        		 *(str + 1) == 'u' &&
                 isxdigit(*(str + 2)) && isxdigit(*(str + 3)) &&
                 isxdigit(*(str + 4)) && isxdigit(*(str + 5))) {
            result.push_back('\\');
            result.append(str, 6);
            str += 6;
            continue;
        }
		else if (ch == '\\') {
			result.push_back('\\');
		}
        result.push_back(ch);
        ++str;
    }
}

void
xscriptXsltEscStr(const char *str, std::string &result) {

    log()->debug("xscriptXsltEscStr: %s", str);
    xscriptTemplatedEscStr(str, "\\/-'\"", result);
}

void
xscriptXsltJSQuoteStr(const char *str, std::string &result) {

    log()->debug("xscriptXsltJSQuoteStr: %s", str);
    xscriptTemplatedEscStr(str, "\\/-'", result);
}

extern "C" void
xscriptXsltEsc(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:esc");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (1 != nargs) {
        XmlUtils::reportXsltError("xscript:esc: bad param count", ctxt);
        return;
    }

    const char* str = params.str(0);
    if (NULL == str) {
        XmlUtils::reportXsltError("xscript:esc: bad parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    try {
        std::string result;
        xscriptXsltEscStr(str, result);
        valuePush(ctxt, xmlXPathNewCString(result.c_str()));
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:esc: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:esc: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
}

extern "C" void
xscriptXsltJsQuote(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:js-quote");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (1 != nargs) {
        XmlUtils::reportXsltError("xscript:js-quote: bad param count", ctxt);
        return;
    }

    const char* str = params.str(0);

    if (NULL == str) {
        XmlUtils::reportXsltError("xscript:js-quote: bad parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    try {
        std::string result;
        result.push_back('\'');
        xscriptXsltJSQuoteStr(str, result);
        result.push_back('\'');
        valuePush(ctxt, xmlXPathNewCString(result.c_str()));
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:js-quote: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:js-quote: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
}

extern "C" void
xscriptXsltJSONQuote(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:json-quote");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (1 != nargs) {
        XmlUtils::reportXsltError("xscript:json-quote: bad param count", ctxt);
        return;
    }

    const char* str = params.str(0);

    if (NULL == str) {
        XmlUtils::reportXsltError("xscript:json-quote: bad parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    try {
        std::string result;
        result.push_back('"');
        xscriptXsltJSONQuoteStr(str, result);
        result.push_back('"');
        valuePush(ctxt, xmlXPathNewCString(result.c_str()));
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:json-quote: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:json-quote: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
}

extern "C" void
xscriptXsltMD5(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:md5");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (1 != nargs) {
        XmlUtils::reportXsltError("xscript:md5: bad param count", ctxt);
        return;
    }

    const char* str = params.str(0);
    if (NULL == str) {
        XmlUtils::reportXsltError("xscript:md5: bad parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    try {
        std::string result = HashUtils::hexMD5(str);
        valuePush(ctxt, xmlXPathNewCString(result.c_str()));
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:md5: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:md5: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
}

extern "C" void
xscriptXsltWbr(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:wbr");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (2 != nargs) {
        XmlUtils::reportXsltError("xscript:wbr: bad param count", ctxt);
        return;
    }

    const char* str = params.str(0);
    if (NULL == str) {
        XmlUtils::reportXsltError("xscript:wbr: bad first parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    if (*str == '\0') {
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    const char* str2 = params.str(1);
    if (NULL == str2) {
        XmlUtils::reportXsltError("xscript:wbr: bad second parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    long int length;
    try {
        length = boost::lexical_cast<long int>(str2);
    }
    catch (const boost::bad_lexical_cast&) {
        XmlUtils::reportXsltError("xscript:wbr: incorrect length format", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    if (length <= 0) {
        XmlUtils::reportXsltError("xscript:wbr: incorrect length value", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    xsltTransformContextPtr tctx = xsltXPathGetTransformContext(ctxt);
    if (NULL == tctx) {
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    Context* ctx = NULL;
    try {
        ctx = Stylesheet::getContext(tctx);
        XmlNodeSetHelper ret(xmlXPathNodeSetCreate(NULL));

        const char* end = str + strlen(str);
        const char* chunk_begin = str;
        if (end - str > length)  {
            long int char_passed = 0;
            while (str < end) {
                if (isspace(*str)) {
                    char_passed = 0;
                }
                else {
                    if (char_passed == length) {
                        xmlNodePtr node = xmlNewTextLen((xmlChar*)chunk_begin, str - chunk_begin);
                        xmlXPathNodeSetAdd(ret.get(), node);
                        ctx->addNode(node);

                        node = xmlNewNode(NULL, (xmlChar*)"wbr");
                        xmlXPathNodeSetAdd(ret.get(), node);
                        ctx->addNode(node);

                        chunk_begin = str;
                        char_passed = 0;
                    }
                    ++char_passed;
                }
                str = StringUtils::nextUTF8(str);
            }

            if (str > end) {
                throw std::runtime_error("incorrect UTF8 data");
            }
        }

        xmlNodePtr node = xmlNewTextLen((xmlChar*)chunk_begin, end - chunk_begin);
        xmlXPathNodeSetAdd(ret.get(), node);
        ctx->addNode(node);

        xmlXPathReturnNodeSet(ctxt, ret.release());
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:wbr: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:wbr: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
}

extern "C" void
xscriptXsltNl2br(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:nl2br");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (1 != nargs) {
        XmlUtils::reportXsltError("xscript:nl2br: bad param count", ctxt);
        return;
    }

    const char* str = params.str(0);
    if (NULL == str) {
        XmlUtils::reportXsltError("xscript:nl2br: bad first parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    if (*str == '\0') {
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    xsltTransformContextPtr tctx = xsltXPathGetTransformContext(ctxt);
    if (NULL == tctx) {
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    Context* ctx = NULL;
    try {
        ctx = Stylesheet::getContext(tctx);
        XmlNodeSetHelper ret(xmlXPathNodeSetCreate(NULL));

        const char* end = str + strlen(str);
        const char* chunk_begin = str;

        while (str < end) {
            if (*str == '\n') {
                if (str - chunk_begin > 0) {
                    xmlNodePtr node = xmlNewTextLen((xmlChar*)chunk_begin, str - chunk_begin);
                    xmlXPathNodeSetAdd(ret.get(), node);
                    ctx->addNode(node);
                }

                xmlNodePtr node = xmlNewNode(NULL, (xmlChar*)"br");
                xmlXPathNodeSetAdd(ret.get(), node);
                ctx->addNode(node);

                str = StringUtils::nextUTF8(str);
                chunk_begin = str;
            }
            else {
                str = StringUtils::nextUTF8(str);
            }
        }

        if (str > end) {
            throw std::runtime_error("incorrect UTF8 data");
        }

        if (end - chunk_begin > 0) {
            xmlNodePtr node = xmlNewTextLen((xmlChar*)chunk_begin, end - chunk_begin);
            xmlXPathNodeSetAdd(ret.get(), node);
            ctx->addNode(node);
        }

        xmlXPathReturnNodeSet(ctxt, ret.release());
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:nl2br: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:nl2br: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
}

extern "C" void
xscriptXsltRemainedDepth(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:remained-depth");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (0 != nargs) {
        XmlUtils::reportXsltError("xscript:remained-depth: bad param count", ctxt);
        return;
    }

    xsltTransformContextPtr tctx = xsltXPathGetTransformContext(ctxt);
    if (NULL == tctx) {
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    try {
        std::string result = boost::lexical_cast<std::string>(xsltMaxDepth - tctx->templNr);
        valuePush(ctxt, xmlXPathNewCString(result.c_str()));
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:remained-depth: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:remained-depth: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
}

extern "C" void
xscriptXsltLogInfo(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:log-info");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (nargs < 1) {
        XmlUtils::reportXsltError("xscript:log-info: bad param count", ctxt);
        return;
    }

    if (log()->level() < Logger::LEVEL_INFO) {
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    std::string result;
    for(int i = 0; i < nargs; ++i) {
        const char* str = params.str(i);
        if (NULL == str) {
            XmlUtils::reportXsltError("xscript:log-info: bad parameter " +
                boost::lexical_cast<std::string>(i), ctxt);
            xmlXPathReturnEmptyNodeSet(ctxt);
            return;
        }
        result.append(str);
    }

    log()->info("%s", result.c_str());
    xmlXPathReturnEmptyNodeSet(ctxt);
}

extern "C" void
xscriptXsltLogWarn(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:log-warn");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (nargs < 1) {
        XmlUtils::reportXsltError("xscript:log-warn: bad param count", ctxt);
        return;
    }

    if (log()->level() < Logger::LEVEL_WARN) {
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    std::string result;
    for(int i = 0; i < nargs; ++i) {
        const char* str = params.str(i);
        if (NULL == str) {
            XmlUtils::reportXsltError("xscript:log-warn: bad parameter " +
                boost::lexical_cast<std::string>(i), ctxt);
            xmlXPathReturnEmptyNodeSet(ctxt);
            return;
        }
        result.append(str);
    }

    log()->warn("%s", result.c_str());
    xmlXPathReturnEmptyNodeSet(ctxt);
}

extern "C" void
xscriptXsltLogError(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:log-error");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (nargs < 1) {
        XmlUtils::reportXsltError("xscript:log-error: bad param count", ctxt);
        return;
    }

    if (log()->level() < Logger::LEVEL_ERROR) {
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    std::string result;
    for(int i = 0; i < nargs; ++i) {
        const char* str = params.str(i);
        if (NULL == str) {
            XmlUtils::reportXsltError("xscript:log-error: bad parameter " +
                boost::lexical_cast<std::string>(i), ctxt);
            xmlXPathReturnEmptyNodeSet(ctxt);
            return;
        }
        result.append(str);
    }

    log()->error("%s", result.c_str());
    xmlXPathReturnEmptyNodeSet(ctxt);
}

extern "C" void
xscriptExtElementBlock(xsltTransformContextPtr tctx, xmlNodePtr node, xmlNodePtr inst, xsltElemPreCompPtr comp) {
    (void)comp;
    if (tctx == NULL) {
        log()->error("%s: no transformation context", BOOST_CURRENT_FUNCTION);
        return;
    }

    Context *ctx = NULL;
    try {
        ctx = Stylesheet::getContext(tctx);

        if (node == NULL) {
            XmlUtils::reportXsltError("xscript:ExtElementBlock: no current node", tctx);
            return;
        }
        if (inst == NULL) {
            XmlUtils::reportXsltError("xscript:ExtElementBlock: no instruction", tctx);
            return;
        }
        if (tctx->insert == NULL) {
            XmlUtils::reportXsltError("xscript:ExtElementBlock: no insertion point", tctx);
            return;
        }

        Stylesheet *stylesheet = Stylesheet::getStylesheet(tctx);
        if (NULL == ctx || NULL == stylesheet) {
            XmlUtils::reportXsltError("xscript:ExtElementBlock: no context or stylesheet", tctx);
            return;
        }
        Block *block = stylesheet->block(inst);
        if (NULL == block) {
            XmlUtils::reportXsltError("xscript:ExtElementBlock: no block found", tctx);
            return;
        }

        InvokeResult result;
        try {
            block->startTimer(ctx);
            result = block->invoke(ctx);
            if (NULL == result.doc.get()) {
                XmlUtils::reportXsltError("xscript:ExtElementBlock: empty result", tctx);
            }
        }
        catch (const InvokeError &e) {
            result = block->errorResult(e.what(), e.info());
        }
        catch (const std::exception &e) {
            result = block->errorResult(e.what());
        }

        if (NULL != result.doc.get()) {
            xmlAddChild(tctx->insert, xmlCopyNode(xmlDocGetRootElement(result.doc.get()), 1));
        }
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:ExtElementBlock: caught exception: " + std::string(e.what()), tctx);
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:ExtElementBlock: caught unknown exception", tctx);
    }
}

XsltFunctionRegisterer::XsltFunctionRegisterer(const char *name, const char *nsref, xmlXPathFunction func) {
    xsltRegisterExtModuleFunction((const xmlChar*) name, (const xmlChar*) nsref, func);
}

XsltElementRegisterer::XsltElementRegisterer(const char* name, const char* nsref, xsltTransformFunction func) {
    xsltRegisterExtModuleElement((const xmlChar*) name, (const xmlChar*) nsref, NULL, func);
}

void
XsltElementRegisterer::registerBlockInvokation(const char *name, const char *nsref) {
    xsltRegisterExtModuleElement((const xmlChar*) name, (const xmlChar*) nsref, NULL, xscriptExtElementBlock);
}

XsltParamFetcher::XsltParamFetcher(xmlXPathParserContextPtr ctxt, int nargs) {

    if (nargs <= 0) {
        return;
    }

    try {
        strings_.reserve(nargs);
        for (int i = 0; i < nargs; ++i) {
            strings_.push_back(xmlXPathPopString(ctxt));
        }
        std::reverse(strings_.begin(), strings_.end());
    }
    catch (const std::exception &e) {
        clear();
        throw;
    }
}

XsltParamFetcher::~XsltParamFetcher() {
    clear();
}

unsigned int
XsltParamFetcher::size() const {
    return static_cast<unsigned int>(strings_.size());
}

const char*
XsltParamFetcher::str(unsigned int index) const {
    return (const char*) strings_.at(index);
}

void
XsltParamFetcher::clear() {
    for (std::vector<xmlChar*>::iterator i = strings_.begin(), end = strings_.end(); i != end; ++i) {
        xmlFree(*i);
    }
}

XsltExtensions::XsltExtensions() {

    XsltFunctionRegisterer("esc", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltEsc);
    XsltFunctionRegisterer("js-quote", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltJsQuote);
    XsltFunctionRegisterer("json-quote", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltJSONQuote);
    
    XsltFunctionRegisterer("md5", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltMD5);
    XsltFunctionRegisterer("wbr", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltWbr);
    XsltFunctionRegisterer("nl2br", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltNl2br);
    XsltFunctionRegisterer("remained-depth", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltRemainedDepth);

    XsltFunctionRegisterer("sanitize", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltSanitize);

    XsltFunctionRegisterer("urlencode", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltUrlencode);
    XsltFunctionRegisterer("urldecode", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltUrldecode);

    XsltFunctionRegisterer("http-redirect", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltHttpRedirect);
    XsltFunctionRegisterer("http_redirect", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltHttpRedirect);

    XsltFunctionRegisterer("get-state-arg", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltGetStateArg);
    XsltFunctionRegisterer("get_state_arg", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltGetStateArg);
    XsltFunctionRegisterer("get-protocol-arg", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltGetProtocolArg);
    XsltFunctionRegisterer("get-query-arg", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltGetQueryArg);
    XsltFunctionRegisterer("get-cookie", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltGetCookie);
    XsltFunctionRegisterer("get-header", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltGetHeader);

    XsltFunctionRegisterer("http-header-out", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltHttpHeaderOut);
    XsltFunctionRegisterer("http_header_out", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltHttpHeaderOut);

    XsltFunctionRegisterer("set-http-status", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltSetHttpStatus);
    XsltFunctionRegisterer("set_http_status", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltSetHttpStatus);

    XsltFunctionRegisterer("log-info", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltLogInfo);
    XsltFunctionRegisterer("log-warn", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltLogWarn);
    XsltFunctionRegisterer("log-error", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltLogError);
}

} // namespace xscript
