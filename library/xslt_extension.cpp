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

#include "xscript/util.h"
#include "xscript/xml_util.h"
#include "xscript/state.h"
#include "xscript/range.h"
#include "xscript/block.h"
#include "xscript/logger.h"
#include "xscript/context.h"
#include "xscript/encoder.h"
#include "xscript/response.h"
#include "xscript/stylesheet.h"
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

void reportXsltError(const std::string &error, xmlXPathParserContextPtr ctxt);
void reportXsltError(const std::string &error, const Context *ctx);

static XsltExtensions xsltExtensions;

extern "C" void
xscriptXsltHttpHeaderOut(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:http-header-out");
    if (NULL == ctxt) {
        return;
    }
    if (2 != nargs) {
        reportXsltError("xscript:http-header-out: bad param count", ctxt);
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    const char* name = params.str(0);
    const char* value = params.str(1);

    if (NULL == name || NULL == value) {
        reportXsltError("xscript:http-header-out: bad parameter", ctxt);
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
        reportXsltError("xscript:http-header-out: caught exception: " + std::string(e.what()), ctx);
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
    if (1 != nargs) {
        reportXsltError("xscript:http-redirect: bad param count", ctxt);
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    const char* str = params.str(0);
    if (NULL == str) {
        reportXsltError("xscript:http-redirect: bad parameter", ctxt);
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
        reportXsltError("xscript:http-redirect: caught exception: " + std::string(e.what()), ctx);
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
    if (1 != nargs) {
        reportXsltError("xscript:set-http-status: bad param count", ctxt);
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    const char* str = params.str(0);
    if (NULL == str) {
        reportXsltError("xscript:set-http-status: bad parameter", ctxt);
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
        reportXsltError("xscript:set-http-status: caught exception: " + std::string(e.what()), ctx);
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
    if (1 != nargs) {
        reportXsltError("xscript:get-state-arg: bad param count", ctxt);
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    const char* str = params.str(0);
    if (NULL == str) {
        reportXsltError("xscript:get-state-arg: bad parameter", ctxt);
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
        State* state = ctx->state();
        const std::string name(str);
        if (!name.empty() && state->has(name)) {
            valuePush(ctxt, xmlXPathNewCString(state->asString(name).c_str()));
        }
        else {
            xmlXPathReturnEmptyNodeSet(ctxt);
        }
    }
    catch (const std::exception &e) {
        reportXsltError("xscript:get-state-arg: caught exception: " + std::string(e.what()), ctx);
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
    if (1 != nargs) {
        reportXsltError("xscript:sanitize: bad param count", ctxt);
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    const char* str = params.str(0);
    if (NULL == str) {
        reportXsltError("xscript:sanitize: bad parameter", ctxt);
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
        std::stringstream stream;
        stream << "<sanitized>" << XmlUtils::sanitize(str) << "</sanitized>" << std::endl;
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
        reportXsltError("xscript:sanitize: caught exception: " + std::string(e.what()), ctx);
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
    if (2 != nargs) {
        reportXsltError("xscript:urlencode: bad param count", ctxt);
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    const char* value = params.str(1);
    const char* encoding = params.str(0);
    if (NULL == value || NULL == encoding) {
        reportXsltError("xscript:urlencode: bad parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    try {
        std::auto_ptr<Encoder> encoder = Encoder::createEscaping("utf-8", encoding);
        std::string encoded;
        encoder->encode(createRange(value), encoded);
        valuePush(ctxt, xmlXPathNewCString(StringUtils::urlencode(encoded).c_str()));
    }
    catch (const std::exception &e) {
        reportXsltError("xscript:urlencode: caught exception: " + std::string(e.what()), ctxt);
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
    if (2 != nargs) {
        reportXsltError("xscript:urldecode: bad param count", ctxt);
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    const char* value = params.str(1);
    const char* encoding = params.str(0);

    if (NULL == value || NULL == encoding) {
        reportXsltError("xscript:urldecode: bad parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    try {
        std::auto_ptr<Encoder> encoder = Encoder::createEscaping(encoding, "utf-8");

        std::string decoded;
        encoder->encode(StringUtils::urldecode(value), decoded);
        valuePush(ctxt, xmlXPathNewCString(decoded.c_str()));
    }
    catch (const std::exception &e) {
        reportXsltError("xscript:urldecode: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
}


void
xscriptTemplatedEscStr(const char *str, const char *esc_template, std::string &result) {

    log()->debug("templatedEscStr: %s", str);

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
            result.append(1, '\\');
            ch = 'n';
        }
        else {
            const char *res = strchr(esc_template, ch);
            if (NULL != res) {
                result.append(1, '\\');
            }
        }
        result.append(1, ch);
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
    if (1 != nargs) {
        reportXsltError("xscript:esc: bad param count", ctxt);
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    const char* str = params.str(0);
    if (NULL == str) {
        reportXsltError("xscript:esc: bad parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    try {
        std::string result;
        xscriptXsltEscStr(str, result);
        valuePush(ctxt, xmlXPathNewCString(result.c_str()));
    }
    catch (const std::exception &e) {
        reportXsltError("xscript:esc: caught exception: " + std::string(e.what()), ctxt);
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
    if (1 != nargs) {
        reportXsltError("xscript:js-quote: bad param count", ctxt);
        return;
    }

    XsltParamFetcher params(ctxt, nargs);
    const char* str = params.str(0);

    if (NULL == str) {
        reportXsltError("xscript:js-quote: bad parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    try {
        std::string result;
        result.append(1,'\'');
        xscriptXsltJSQuoteStr(str, result);
        result.append(1, '\'');
        valuePush(ctxt, xmlXPathNewCString(result.c_str()));
    }
    catch (const std::exception &e) {
        reportXsltError("xscript:js-quote: caught exception: " + std::string(e.what()), ctxt);
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
    if (1 != nargs) {
        reportXsltError("xscript:md5: bad param count", ctxt);
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    const char* str = params.str(0);
    if (NULL == str) {
        reportXsltError("xscript:md5: bad parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    try {
        std::string result = HashUtils::hexMD5(str);
        valuePush(ctxt, xmlXPathNewCString(result.c_str()));
    }
    catch (const std::exception &e) {
        reportXsltError("xscript:md5: caught exception: " + std::string(e.what()), ctxt);
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

    if (2 != nargs) {
        reportXsltError("xscript:wbr: bad param count", ctxt);
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    const char* str = params.str(0);
    if (NULL == str) {
        reportXsltError("xscript:wbr: bad first parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    if (*str == '\0') {
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    const char* str2 = params.str(1);
    if (NULL == str2) {
        reportXsltError("xscript:wbr: bad second parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    long int length;
    try {
        length = boost::lexical_cast<long int>(str2);
    }
    catch (const boost::bad_lexical_cast&) {
        reportXsltError("xscript:wbr: incorrect length format", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    if (length <= 0) {
        reportXsltError("xscript:wbr: incorrect length value", ctxt);
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
        reportXsltError("xscript:wbr: caught exception: " + std::string(e.what()), ctx);
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

    if (1 != nargs) {
        reportXsltError("xscript:nl2br: bad param count", ctxt);
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    const char* str = params.str(0);
    if (NULL == str) {
        reportXsltError("xscript:nl2br: bad first parameter", ctxt);
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
        reportXsltError("xscript:nl2br: caught exception: " + std::string(e.what()), ctx);
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
    if (0 != nargs) {
        reportXsltError("xscript:remained-depth: bad param count", ctxt);
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
        reportXsltError("xscript:remained-depth: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
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
            reportXsltError("xscript:ExtElementBlock: no current node", ctx);
            return;
        }
        if (inst == NULL) {
            reportXsltError("xscript:ExtElementBlock: no instruction", ctx);
            return;
        }
        if (tctx->insert == NULL) {
            reportXsltError("xscript:ExtElementBlock: no insertion point", ctx);
            return;
        }
        
        Stylesheet *stylesheet = Stylesheet::getStylesheet(tctx);
        if (NULL == ctx || NULL == stylesheet) {
            reportXsltError("xscript:ExtElementBlock: no context or stylesheet", ctx);
            return;
        }
        Block *block = stylesheet->block(inst);
        if (NULL == block) {
            reportXsltError("xscript:ExtElementBlock: no block found", ctx);
            return;
        }

        XmlDocHelper doc;
        try {
            block->startTimer(ctx);
            doc = block->invoke(ctx);
            if (NULL == doc.get()) {
                reportXsltError("xscript:ExtElementBlock: empty result", ctx);
            }
        }
        catch (const InvokeError &e) {
            doc = block->errorResult(e.what(), e.info());
        }
        catch (const std::exception &e) {
            doc = block->errorResult(e.what());
        }

        if (NULL != doc.get()) {
            xmlAddChild(tctx->insert, xmlCopyNode(xmlDocGetRootElement(doc.get()), 1));
        }
    }
    catch (const std::exception &e) {
        reportXsltError("xscript:ExtElementBlock: caught exception: " + std::string(e.what()), ctx);
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

void
reportXsltError(const std::string &error, xmlXPathParserContextPtr ctxt) {
    xsltTransformContextPtr tctx = xsltXPathGetTransformContext(ctxt);

    const Context *ctx = NULL;
    if (NULL != tctx) {
        try {
            ctx = Stylesheet::getContext(tctx);
        }
        catch(const std::exception &e) {
            log()->error("caught exception during handling of error: %s. Exception: %s", error.c_str(), e.what());
            return;
        }
    }

    reportXsltError(error, ctx);

}

void
reportXsltError(const std::string &error, const Context *ctx) {
    if (NULL == ctx) {
        log()->error("%s", error.c_str());
    }
    else {
        log()->error("%s. Owner: %s", error.c_str(), ctx->xsltName().c_str());
    }
}


XsltExtensions::XsltExtensions() {

    XsltFunctionRegisterer("esc", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltEsc);
    XsltFunctionRegisterer("js-quote", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltJsQuote);
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

    XsltFunctionRegisterer("http-header-out", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltHttpHeaderOut);
    XsltFunctionRegisterer("http_header_out", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltHttpHeaderOut);

    XsltFunctionRegisterer("set-http-status", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltSetHttpStatus);
    XsltFunctionRegisterer("set_http_status", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltSetHttpStatus);
}

} // namespace xscript
