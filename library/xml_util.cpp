#include "settings.h"

#include <cctype>
#include <cstdarg>
#include <cassert>
#include <stdexcept>
#include <iostream>

#include <boost/thread/tss.hpp>
#include <boost/current_function.hpp>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlerror.h>

#include <libxslt/xsltutils.h>
#include <libxslt/xsltInternals.h>

#include <libexslt/exslt.h>
#include <libexslt/exsltconfig.h>

#include "xscript/util.h"
#include "xscript/xml_util.h"
#include "xscript/range.h"
#include "xscript/logger.h"
#include "xscript/encoder.h"
#include "xscript/sanitizer.h"
#include "internal/algorithm.h"
#include "xscript/xml_helpers.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

static const Range ESCAPE_PATTERN = createRange("&'\"<>");
const char * const XmlUtils::XSCRIPT_NAMESPACE = "http://www.yandex.ru/xscript";

class XmlErrorReporter {
public:
    XmlErrorReporter();

    void reset();
    const std::string& message() const;

    void report(const char *format, ...);
    void report(const char *format, va_list args);

    bool hasError() const {
        return error_;
    }

    static XmlErrorReporter* reporter();

private:
    bool error_;
    std::string message_;

    static boost::thread_specific_ptr<XmlErrorReporter> reporter_;
    static const std::string UNKNOWN_XML_ERROR;
};

boost::thread_specific_ptr<XmlErrorReporter> XmlErrorReporter::reporter_;
const std::string XmlErrorReporter::UNKNOWN_XML_ERROR = "unknown XML error";

extern "C" void xmlNullError(void *, const char *, ...);
extern "C" void xmlReportPlainError(void *, const char *, ...);
extern "C" void xmlReportStructuredError(void *, xmlErrorPtr error);

XmlUtils::XmlUtils() {

    xmlInitParser();
    xmlLineNumbersDefault(1);

    xmlLoadExtDtdDefaultValue |= XML_DETECT_IDS;
    xmlLoadExtDtdDefaultValue |= XML_COMPLETE_ATTRS;
    xmlSubstituteEntitiesDefault(1); // 0

    exsltStrRegister();
    exsltDynRegister();

    exsltMathRegister();
    exsltSetsRegister();
    exsltFuncRegister();

    exsltDateRegister();
    exsltSaxonRegister();
    exsltCommonRegister();

    registerReporters();
}

XmlUtils::~XmlUtils() {
    xsltCleanupGlobals();
    xmlCleanupParser();
}

void
XmlUtils::init(const Config *config) {
    xsltMaxDepth = config->as<int>("/xscript/xslt/max-depth", 300);
}

void
XmlUtils::registerReporters() {
    //xmlSetGenericErrorFunc(NULL, &xmlNullError);
    xmlSetGenericErrorFunc(NULL, &xmlReportPlainError);
    xsltSetGenericErrorFunc(NULL, &xmlReportPlainError);
    //xmlSetStructuredErrorFunc(NULL, &xmlReportStructuredError);

    resetReporter();
}

void
XmlUtils::resetReporter() {
    XmlErrorReporter::reporter()->reset();
}

void
XmlUtils::throwUnless(bool value) {
    XmlErrorReporter *rep = XmlErrorReporter::reporter();
    if (!value) {
        UnboundRuntimeError e(rep->message());
        resetReporter();
        throw e;
    }
}

bool
XmlUtils::hasXMLError() {
    return XmlErrorReporter::reporter()->hasError();
}

void
XmlUtils::printXMLError(const std::string& postfix) {
    if (XmlErrorReporter::reporter()->hasError()) {
        log()->error("Got XML error: %s. %s", XmlErrorReporter::reporter()->message().c_str(), postfix.c_str());
        resetReporter();
    }
}

std::string
XmlUtils::escape(const Range &range) {

    std::string dest;
    dest.reserve(range.size());

    Range::const_iterator begin = range.begin(), end = range.end();
    Range::const_iterator pb = ESCAPE_PATTERN.begin(), pe = ESCAPE_PATTERN.end();

    while (true) {
        Range::const_iterator pos = std::find_first_of(begin, end, pb, pe);
        if (end == pos) {
            dest.append(begin, end);
            break;
        }
        dest.append(begin, pos);
        switch (*pos) {
        case '<':
            dest.append("&lt;");
            break;
        case '>':
            dest.append("&gt;");
            break;

        case '"':
            dest.append("&quot;");
            break;
        case '\'':  {
            dest.append("&apos;");
        }
        case '&': {
            dest.append("&amp;");
            break;
        }
        default:
            dest.push_back(*pos);
            break;
        }
        begin = pos + 1;
    }
    return dest;
}

std::string
XmlUtils::sanitize(const Range &range) {
    return Sanitizer::instance()->sanitize(range);
}

bool
XmlUtils::xpathExists(xmlDocPtr doc, const std::string &path) {

    XmlXPathContextHelper ctx(xmlXPathNewContext(doc));
    throwUnless(NULL != ctx.get());

    XmlXPathObjectHelper object(xmlXPathEvalExpression((const xmlChar*) path.c_str(), ctx.get()));
    throwUnless(NULL != object.get());
    return (NULL != object->nodesetval && 0 != object->nodesetval->nodeNr);
}

std::string
XmlUtils::xpathValue(xmlDocPtr doc, const std::string &path, const std::string &defval) {

    std::string res = defval;
    XmlXPathContextHelper ctx(xmlXPathNewContext(doc));
    throwUnless(NULL != ctx.get());

    XmlXPathObjectHelper object(xmlXPathEvalExpression((const xmlChar*) path.c_str(), ctx.get()));
    throwUnless(NULL != object.get());

    if (NULL != object->nodesetval && 0 != object->nodesetval->nodeNr) {
        const char *val = value(object->nodesetval->nodeTab[0]);
        if (NULL != val) {
            res.assign(val);
        }
    }
    return res;
}

XmlErrorReporter::XmlErrorReporter() :
        error_(false) {}

void
XmlErrorReporter::reset() {
    error_ = false;
    message_.clear();
}

const std::string&
XmlErrorReporter::message() const {
    return error_ ? message_ : UNKNOWN_XML_ERROR;
}

void
XmlErrorReporter::report(const char *format, ...) {
    va_list args;
    va_start(args, format);
    report(format, args);
    log()->xmllog(format, args);
    va_end(args);
}

void
XmlErrorReporter::report(const char *format, va_list args) {
    char part_message[5120];
    vsnprintf(part_message, sizeof(part_message), format, args);
    part_message[5119] = 0;
    message_ += part_message;
    error_ = true;
}

XmlErrorReporter*
XmlErrorReporter::reporter() {
    XmlErrorReporter *rep = reporter_.get();
    if (NULL == rep) {
        rep = new XmlErrorReporter();
        reporter_.reset(rep);
    }
    return rep;
}

extern "C" void
xmlNullError(void *ctx, const char *format, ...) {
    (void)ctx;
    (void)format;
}

extern "C" void
xmlReportPlainError(void *ctx, const char *format, ...) {
    (void)ctx;
    try {
        va_list args;
        va_start(args, format);
        XmlErrorReporter::reporter()->report(format, args);
        va_end(args);
    }
    catch (const std::exception &e) {
        log()->error("%s, caught exception: %s", BOOST_CURRENT_FUNCTION, e.what());
    }
}

extern "C" void
xmlReportStructuredError(void *ctx, xmlErrorPtr error) {
    (void)ctx;
    try {
        const char *message = error->message ? error->message : "unknown XML error";
        XmlErrorReporter::reporter()->report("%s line %d", message, error->line);
    }
    catch (const std::exception &e) {
        log()->error("%s, caught exception: %s", BOOST_CURRENT_FUNCTION, e.what());
    }
}

static XmlUtils utils_;

} // namespace xscript
