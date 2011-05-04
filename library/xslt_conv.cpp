#include "settings.h"

#include <exception>

#include <libxml/xpathInternals.h>

#include "xscript/encoder.h"
#include "xscript/logger.h"
#include "xscript/xml_util.h"
#include "xscript/xslt_extension.h"
#include "xscript/punycode_utils.h"
#include "xscript/string_utils.h"


#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class XsltConv {
public:
    XsltConv();
};

static XsltConv xsltConv;


extern "C" void
xscriptXsltPunycodeDomainEncode(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:punycode-domain-encode");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (nargs < 1 || nargs > 2) {
        XmlUtils::reportXsltError("xscript:punycode-domain-encode: bad param count", ctxt);
        return;
    }

    const char* value = params.str(0);
    const char* encoding = NULL;
    if (nargs == 2) {
        encoding = params.str(1);
    }

    if (NULL == value || (2 == nargs && NULL == encoding)) {
        XmlUtils::reportXsltError("xscript:punycode-domain-encode: bad parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    if (!*value) {
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    try {
        PunycodeEncoder encoder(encoding);
        std::string result;
        encoder.domainEncode(createRange(value), result);
        valuePush(ctxt, xmlXPathNewCString(result.c_str()));
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:punycode-domain-encode: caught exception: " + std::string(e.what()), ctxt);

        // bad input data?
        // ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:punycode-domain-encode: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
}

extern "C" void
xscriptXsltPunycodeDomainDecode(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:punycode-domain-decode");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (nargs < 1 || nargs > 2) {
        XmlUtils::reportXsltError("xscript:punycode-domain-decode: bad param count", ctxt);
        return;
    }

    const char* value = params.str(0);
    const char* encoding = NULL;
    if (nargs == 2) {
        encoding = params.str(1);
    }

    if (NULL == value || (2 == nargs && NULL == encoding)) {
        XmlUtils::reportXsltError("xscript:punycode-domain-decode: bad parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    if (!*value) {
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    try {
        PunycodeDecoder encoder(encoding);
        std::string result;
        encoder.domainDecode(createRange(value), result);
        valuePush(ctxt, xmlXPathNewCString(result.c_str()));
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:punycode-domain-decode: caught exception: " + std::string(e.what()), ctxt);

        // bad input data?
        // ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:punycode-domain-decode: caught unknown exception", ctxt);
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

        // bad input data?
        // ctxt->error = XPATH_EXPR_ERROR;
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

        // bad input data?
        // ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:urldecode: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
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
        Range range = createRange(str);
        result.reserve(range.size() * 2);

        StringUtils::escapeString(range, result);
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
        Range range = createRange(str);
        result.reserve(range.size() * 2 + 2);

        result.push_back('\'');
        StringUtils::jsQuoteString(range, result);
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
        Range range = createRange(str);
        result.reserve(range.size() * 2 + 2);

        result.push_back('"');
        StringUtils::jsonQuoteString(range, result);
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
xscriptXsltXmlEscape(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:xmlescape");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    if (1 != nargs) {
        XmlUtils::reportXsltError("xscript:xmlescape: bad param count", ctxt);
        return;
    }

    const char* str = params.str(0);
    if (NULL == str) {
        XmlUtils::reportXsltError("xscript:xmlescape: bad parameter", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    try {
        valuePush(ctxt, xmlXPathNewCString(XmlUtils::escape(str).c_str()));
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:xmlescape: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:xmlescape: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
}


XsltConv::XsltConv() {

    XsltFunctionRegisterer("esc", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltEsc);
    XsltFunctionRegisterer("js-quote", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltJsQuote);
    XsltFunctionRegisterer("json-quote", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltJSONQuote);

    XsltFunctionRegisterer("urlencode", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltUrlencode);
    XsltFunctionRegisterer("urldecode", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltUrldecode);

    XsltFunctionRegisterer("punycode-domain-encode", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltPunycodeDomainEncode);
    XsltFunctionRegisterer("punycode-domain-decode", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltPunycodeDomainDecode);

    XsltFunctionRegisterer("xmlescape", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltXmlEscape);
}

} // namespace xscript
