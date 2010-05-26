#include "settings.h"

#include <cctype>
#include <cstdarg>
#include <cassert>

#include <boost/thread/tss.hpp>
#include <boost/current_function.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/exception.hpp>

#include <libxml/parser.h>
#include <libxml/xmlerror.h>

#include <libxslt/extensions.h>
#include <libxslt/xsltutils.h>
#include <libxslt/xsltInternals.h>

#include <libexslt/exslt.h>
#include <libexslt/exsltconfig.h>

#include "internal/algorithm.h"
#include "xscript/block.h"
#include "xscript/context.h"
#include "xscript/encoder.h"
#include "xscript/exception.h"
#include "xscript/logger.h"
#include "xscript/policy.h"
#include "xscript/range.h"
#include "xscript/sanitizer.h"
#include "xscript/script.h"
#include "xscript/stylesheet.h"
#include "xscript/string_utils.h"
#include "xscript/vhost_data.h"
#include "xscript/xml_helpers.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

static const Range ESCAPE_PATTERN = createRange("&'\"<>");
const char * const XmlUtils::XSCRIPT_NAMESPACE = "http://www.yandex.ru/xscript";
static xmlExternalEntityLoader external_entity_default_loader_ = NULL;

static boost::thread_specific_ptr<Xml::TimeMapType> xml_info_collector_modified_info_;
static boost::thread_specific_ptr<XmlInfoCollector::ErrorMapType> xml_info_collector_error_info_;

static int xmlVersion = 0;
static int xsltVersion = 0;
static int exsltVersion = 0;

XmlDocHelper
XmlUtilCreateFakeDoc() {
    XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
    if (NULL != doc.get()) {
        XmlNodeHelper node(xmlNewDocNode(doc.get(), NULL, (const xmlChar*)"fake", NULL));
        if (node.get() != NULL) {
            xmlDocSetRootElement(doc.get(), node.release());
        }
    }
    return doc;
}

static XmlDocHelper xml_fake_doc_(XmlUtilCreateFakeDoc());


class XmlErrorReporter {
public:
    XmlErrorReporter();

    void reset();
    const std::string& message() const;

    void report(const char *format, ...);
    void report(const char *format, va_list args);

    bool hasError() const;

    static XmlErrorReporter* reporter();
    
private:
    std::string message_;
    bool error_;

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

    exsltRegisterAll();

    registerReporters();

    xscript::xmlVersion = boost::lexical_cast<int>(xmlParserVersion);
    xscript::xsltVersion = boost::lexical_cast<int>(xsltEngineVersion);
    xscript::exsltVersion = boost::lexical_cast<int>(exsltLibraryVersion);
}

XmlUtils::~XmlUtils() {
    xsltCleanupGlobals();
    xmlCleanupParser();
}

void
XmlUtils::init(const Config *config) {
    xsltMaxDepth = config->as<int>("/xscript/xslt/max-depth", 300);

    xmlExternalEntityLoader xml_loader = xmlGetExternalEntityLoader();

    if(xml_loader != XmlUtils::entityResolver) {
        xmlSetExternalEntityLoader(XmlUtils::entityResolver);
        external_entity_default_loader_ = xml_loader;
    }
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

void
XmlUtils::throwUnless(bool value, const char *attr, const char *attr_value) {
    try {
        throwUnless(value);
    }
    catch(UnboundRuntimeError &error) {
        std::string message(" ");
        message.append(attr);
        message.append(": ");
        message.append(attr_value);
        error.append(message);
        throw error;
    }
}

bool
XmlUtils::hasXMLError() {
    return XmlErrorReporter::reporter()->hasError();
}

void
XmlUtils::printXMLError(const std::string& postfix) {
    if (XmlErrorReporter::reporter()->hasError()) {
        log()->error("Got XML error: %s %s", XmlErrorReporter::reporter()->message().c_str(), postfix.c_str());
        resetReporter();
    }
}

std::string
XmlUtils::getXMLError() {
    if (XmlErrorReporter::reporter()->hasError()) {
        std::string error = XmlErrorReporter::reporter()->message();
        resetReporter();
        return error;
    }
    return StringUtils::EMPTY_STRING;
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
        case '\'':
            dest.append("&#39;");
            break;
        case '&':
            dest.append("&amp;");
            break;
        default:
            dest.push_back(*pos);
            break;
        }
        begin = pos + 1;
    }
    return dest;
}

bool
XmlUtils::validate(const std::string &data) {
    const char* str = data.c_str();
    const char* end = str + data.length();
    while(str < end) {
        const char* chunk_begin = str;
        str = StringUtils::nextUTF8(str);
        if (str - chunk_begin > 1) {
            continue;
        }
        char ch = static_cast<unsigned char>(*chunk_begin);
        if (ch > 0x1F || ch == 0x9 || ch == 0xA || ch == 0xD) {
            continue;
        }           
        return false;
    }
    return true;
}

std::string
XmlUtils::sanitize(const Range &range, const std::string &base_url, int line_limit) {
    return Sanitizer::instance()->sanitize(range, base_url, line_limit);
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

void
XmlUtils::reportXsltError(const std::string &error, xmlXPathParserContextPtr ctxt) {
    xsltTransformContextPtr tctx = xsltXPathGetTransformContext(ctxt);
    reportXsltError(error, tctx);
}

void
XmlUtils::reportXsltError(const std::string &error, xsltTransformContextPtr tctx) {
    reportXsltError(error, tctx, true);
}

void
XmlUtils::reportXsltError(const std::string &error, xsltTransformContextPtr tctx, bool strong_check) {

    Context *ctx = NULL;
    if (NULL != tctx) {
        try {
            ctx = Stylesheet::getContext(tctx).get();
            if (strong_check) {
                ctx->assignRuntimeError(Stylesheet::getBlock(tctx), error);
            }
            else {
                ctx->setNoCache();
            }
        }
        catch(const std::exception &e) {
            log()->error("caught exception during handling of error: %s. Exception: %s", error.c_str(), e.what());
            return;
        }
    }

    if (NULL == ctx) {
        log()->error("%s", error.c_str());
    }
    else {
        log()->error("%s. Script: %s. Stylesheet: %s",
            error.c_str(), ctx->script()->name().c_str(), ctx->xsltName().c_str());
    }
}

static std::string
getOriginalUrl() {
    const Request* request = VirtualHostData::instance()->get();
    if (NULL == request) {
        return StringUtils::EMPTY_STRING;
    }
    return request->getOriginalUrl();
}

xmlParserInputPtr
XmlUtils::entityResolver(const char *url, const char *id, xmlParserCtxtPtr ctxt) {
    if (NULL == url) {
        url = "";
    }
    if (NULL == id) {
        id = "";
    }
    log()->info("entityResolver. url: %s, id: %s", url, id);

    try {
        XmlInfoCollector::ErrorMapType* error_info = XmlInfoCollector::getErrorInfo();
        if (external_entity_default_loader_ == NULL) {
            std::string error = std::string("Default entity loader not set. URL: ") + url;
            if ('\0' != *id) {
                error.append(". ID: ").append(id);
            }
            if (error_info) {
                error_info->insert(std::make_pair(url, error));
            }
            log()->error("%s", error.c_str());
            return NULL;
        }

        std::string fileName;
        if ('\0' != *url) {
            if (strncasecmp(url, "http://", sizeof("http://") - 1) == 0 ||
                strncasecmp(url, "https://", sizeof("https://") - 1) == 0) {
                log()->warn("entityResolver: loading remote entity is forbidden. URL: %s. ID: %s. Request URL: %s",
                        url, id, getOriginalUrl().c_str());
                return NULL;
            }

            fileName = Policy::getPathByScheme(NULL, url);
            if (fileName != url) {
                log()->info("entityResolver: url changed %s", fileName.c_str());
            }
        }
        try {
            XmlErrorReporter *reporter = XmlErrorReporter::reporter();
            bool had_error = reporter->hasError();
            xmlParserInputPtr ret = external_entity_default_loader_(fileName.c_str(), id, ctxt);
            if (!had_error && reporter->hasError()) {
                const std::string &message = reporter->message();
                if (std::string::npos != message.find("failed to load external entity") &&
                    std::string::npos == message.find("no fallback was found")) {
                    reporter->reset();
                }
            }
            if (ret == NULL) {
                std::string error = std::string("Cannot resolve external entity: ") + url;
                if ('\0' != *id) {
                    error.append(". ID: ").append(id);
                }
                error.append(". Request URL: ").append(getOriginalUrl());
                
                Xml::TimeMapType* modified_info = XmlInfoCollector::getModifiedInfo();
                if (NULL != modified_info) {
                    boost::filesystem::path path(fileName);
                    modified_info->insert(std::make_pair(path.native_file_string(), 0));
                }
                log()->warn("%s", error.c_str());
                return NULL;
            }
            
            if (fileName.find("://") == std::string::npos) {
                namespace fs = boost::filesystem;
                try {
                    Xml::TimeMapType* modified_info = XmlInfoCollector::getModifiedInfo();
                    if (NULL != modified_info) {
                        fs::path path(fileName);
                        if (fs::exists(path) && !fs::is_directory(path)) {
                            time_t modified = fs::last_write_time(path);
                            modified_info->insert(std::make_pair(path.native_file_string(), modified));
                        }
                    }
                }
                catch (const fs::filesystem_error &e) {
                    std::string error = std::string("Cannot stat file: ") + e.what() + url;
                    if ('\0' != *id) {
                        error.append(". ID: ").append(id);
                    }
                    if (error_info) {
                        error_info->insert(std::make_pair(fileName, error));
                    }
                    log()->error("%s", error.c_str());
                }
            }
            return ret;
        }
        catch (const std::exception &e) {
            std::string error = std::string("Entity resolver error: ") + e.what() + url;
            if ('\0' != *id) {
                error.append(". ID: ").append(id);
            }
            
            error.append(". Request URL: ").append(getOriginalUrl());
            
            if (error_info) {
                error_info->insert(std::make_pair(fileName, error));
            }
            
            log()->error("%s", error.c_str());
        }
        catch (...) {
            std::string error = std::string("Entity resolver unknown error. URL: ") + url;
            if ('\0' != *id) {
                error.append(". ID: ").append(id);
            }
            
            error.append(". Request URL: ").append(getOriginalUrl());
            
            if (error_info) {
                error_info->insert(std::make_pair(fileName, error));
            }
            log()->error("%s", error.c_str());
        }
    }
    catch (const std::exception &e) {
        log()->error("entityResolver error: %s. URL: %s. ID: %s. Request URL: %s", e.what(), url, id, getOriginalUrl().c_str());
    }
    catch (...) {
        log()->error("entityResolver unknown error. URL: %s. ID: %s. Request URL: %s", url, id, getOriginalUrl().c_str());
    }

    return NULL;
}

xmlDocPtr
XmlUtils::fakeXml() {
    return xml_fake_doc_.get();
}

static bool
traverseNode(xmlNodePtr target, xmlNodePtr node, boost::int32_t &count) {
    while(node) {
        ++count;
        if (node == target) {
            return true;
        }
        if (node->children) {
            bool res = traverseNode(target, node->children, count);
            if (res) {
                return true;
            }
        }
        node = node->next;
    }
    return false;
}

boost::int32_t
XmlUtils::getNodeCount(xmlNodePtr node) {
    xmlNodePtr root = node;
    while(root->parent) {
        root = root->parent;
    };
    
    boost::int32_t count = 0;
    bool res = traverseNode(node, root, count);
    return res ? count : -1;
}

int
XmlUtils::xmlVersionNumber() {
    return xscript::xmlVersion;
}

int
XmlUtils::xsltVersionNumber() {
    return xscript::xsltVersion;
}

int
XmlUtils::exsltVersionNumber() {
    return xscript::exsltVersion;
}

const char*
XmlUtils::xmlVersion() {
    return xmlParserVersion;
}

const char*
XmlUtils::xsltVersion() {
    return xsltEngineVersion;
}

const char*
XmlUtils::exsltVersion() {
    return exsltLibraryVersion;
}

XmlInfoCollector::XmlInfoCollector() {
}

void
XmlInfoCollector::ready(bool flag) {
    if (flag) {
        xml_info_collector_modified_info_.reset(new Xml::TimeMapType());
        xml_info_collector_error_info_.reset(new ErrorMapType());
    }
    else {
        xml_info_collector_modified_info_.reset(NULL);
        xml_info_collector_error_info_.reset(NULL);
    }
}

Xml::TimeMapType*
XmlInfoCollector::getModifiedInfo() {
    return xml_info_collector_modified_info_.get();
}

XmlInfoCollector::ErrorMapType*
XmlInfoCollector::getErrorInfo() {
    return xml_info_collector_error_info_.get();
}

std::string
XmlInfoCollector::getError() {
    ErrorMapType *info = xml_info_collector_error_info_.get();
    if (info == NULL) {
        return StringUtils::EMPTY_STRING;
    }
    std::string error;
    for(XmlInfoCollector::ErrorMapType::const_iterator it = info->begin();
        it != info->end();
        ++it) {
        error.append("Error while process file: ");
        error.append(it->first);
        error.append(". ");
        error.append(it->second);
        error.append(". ");
    }
    return error;
}

XmlInfoCollector::Starter::Starter() {
    XmlInfoCollector::ready(true);
}

XmlInfoCollector::Starter::~Starter() {
    XmlInfoCollector::ready(false);
}

XmlErrorReporter::XmlErrorReporter() : error_(false)
{}

void
XmlErrorReporter::reset() {
    message_.clear();
    error_ = false;
}

const std::string&
XmlErrorReporter::message() const {
    return message_.empty() ? UNKNOWN_XML_ERROR : message_;
}

bool
XmlErrorReporter::hasError() const {
    return error_;
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
