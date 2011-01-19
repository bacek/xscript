#include "settings.h"

#include <exception>

#include <libxml/xpathInternals.h>

#include "xscript/encoder.h"
#include "xscript/logger.h"
#include "xscript/xml_util.h"
#include "xscript/xslt_extension.h"
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

static void
xscriptTemplatedEscStr(const char *str, const char *esc_template, bool js_check, std::string &result) {
    const char* end = str + strlen(str);
    const char* str_next = str;
    while (str_next < end) {
        str = str_next;
        str_next = StringUtils::nextUTF8(str);
        unsigned char ch = *str;
        if (ch == '\r' && end - str > 0 && *(str + 1) == '\n') {
            result.append("\\n");
            ++str;
            ++str_next;
            continue;
        }
        else if (ch == '\r' || ch == '\n') {
            result.append("\\n");
            continue;
        }
        else if (str_next - str == 1 && NULL != strchr(esc_template, ch)) {
            result.push_back('\\');
        }
        else if (js_check && str_next - str > 1) {
            if (str_next - str == 2) {
                unsigned char ch2 = *(str + 1);
                if (ch == 0xC2 && ch2 == 0x85) {
                    result.append("\\u0085");
                    continue;
                }
            }
            else if (str_next - str == 3) {
                unsigned char ch2 = *(str + 1);
                unsigned char ch3 = *(str + 2);
                if (ch == 0xE2 && ch2 == 0x80 && ch3 == 0xA8) {
                    result.append("\\u2028");
                    continue;                    
                }
                else if (ch == 0xE2 && ch2 == 0x80 && ch3 == 0xA9) {
                    result.append("\\u2029");
                    continue;                    
                }
            }
        }
        result.append(str, str_next - str);
    }
}

static void
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
        else if (ch == '\\' && end - str > 5 && *(str + 1) == 'u' &&
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

static void
xscriptXsltEscStr(const char *str, std::string &result) {

    log()->debug("xscriptXsltEscStr: %s", str);
    xscriptTemplatedEscStr(str, "\\/-'\"", false, result);
}

static void
xscriptXsltJSQuoteStr(const char *str, std::string &result) {

    log()->debug("xscriptXsltJSQuoteStr: %s", str);
    xscriptTemplatedEscStr(str, "\\/-'", true, result);
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

    XsltFunctionRegisterer("xmlescape", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltXmlEscape);
}

} // namespace xscript
