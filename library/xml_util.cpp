#include "settings.h"

#include <cctype>
#include <cstdarg>
#include <cassert>

#include <boost/thread/tss.hpp>
#include <boost/current_function.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/exception.hpp>

#include <libxml/globals.h>
#include <libxml/parser.h>
#include <libxml/xmlerror.h>
#include <libxml/xpathInternals.h>

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

static const std::string STR_ERROR_CANNOT_STAT_FILE = "Cannot stat file: ";
static const std::string STR_ENTITY_RESOLVER_ERROR = "Entity resolver error: ";
static const std::string STR_ENTITY_RESOLVER_UNKNOWN_ERROR = "Entity resolver unknown error.";
static const std::string STR_ENTITY_RESOLVER_LOADING_FAILED = "failed to load external entity";
static const std::string STR_ENTITY_RESOLVER_NO_FALLBACK = "no fallback was found";

static const std::string ESCAPE_PATTERN_STR = "&'\"<>";
static const Range ESCAPE_PATTERN = createRange(ESCAPE_PATTERN_STR);
const char * const XmlUtils::XSCRIPT_NAMESPACE = "http://www.yandex.ru/xscript";
static xmlExternalEntityLoader external_entity_default_loader_ = NULL;

static xmlParserInputBufferCreateFilenameFunc old_libxml_file_resolver_ = NULL;
static xmlParserInputBufferPtr XmlUtilsFileResolver(const char *URI, xmlCharEncoding enc);

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
    static const unsigned int CHUNK_MESSAGE_SIZE;
    static const unsigned int MESSAGE_SIZE_LIMIT;
};

boost::thread_specific_ptr<XmlErrorReporter> XmlErrorReporter::reporter_;
const std::string XmlErrorReporter::UNKNOWN_XML_ERROR = "unknown XML error";
const unsigned int XmlErrorReporter::CHUNK_MESSAGE_SIZE = 5120;
const unsigned int XmlErrorReporter::MESSAGE_SIZE_LIMIT = 10 * 1024;

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
        assert(NULL != external_entity_default_loader_);
    }

    xmlParserInputBufferCreateFilenameFunc libxml_file_resolver =
    	xmlParserInputBufferCreateFilenameDefault(XmlUtilsFileResolver);
    if (libxml_file_resolver != XmlUtilsFileResolver) {
        old_libxml_file_resolver_ = libxml_file_resolver;
        assert(NULL != old_libxml_file_resolver_);

        xmlParserInputBufferCreateFilenameFunc libxml_thr_file_resolver =
            xmlThrDefParserInputBufferCreateFilenameDefault(XmlUtilsFileResolver);
        assert(libxml_thr_file_resolver == old_libxml_file_resolver_);
    }
    assert(xmlParserInputBufferCreateFilenameDefault(XmlUtilsFileResolver) == XmlUtilsFileResolver);
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

void
XmlUtils::escape(const Range &range, std::string &dest) {

    Range::const_iterator begin = range.begin(), end = range.end();
    Range::const_iterator pb = ESCAPE_PATTERN.begin(), pe = ESCAPE_PATTERN.end();

    while (begin < end) {
        Range::const_iterator pos = std::find_first_of(begin, end, pb, pe);
        if (end == pos) {
            dest.append(begin, end);
            break;
        }
        dest.append(begin, pos);
        switch (*pos) {
        case '<':
            dest.append("&lt;", 4);
            break;
        case '>':
            dest.append("&gt;", 4);
            break;
        case '"':
            dest.append("&quot;", 6);
            break;
        case '\'':
            dest.append("&#39;", 5);
            break;
        case '&':
            dest.append("&amp;", 5);
            break;
        default:
            dest.push_back(*pos);
            break;
        }
        begin = pos + 1;
    }
}

std::string
XmlUtils::escape(const Range &range) {

    if (range.empty()) {
        return StringUtils::EMPTY_STRING;
    }

    std::string dest;
    dest.reserve(range.size() + range.size() / 4);
    escape(range, dest);
    return dest;
}

std::string
XmlUtils::escape(const std::string &str) {

    if (str.empty()) {
        return StringUtils::EMPTY_STRING;
    }

    std::string::const_iterator begin = str.begin(), end = str.end();
    std::string::const_iterator pb = ESCAPE_PATTERN_STR.begin(), pe = ESCAPE_PATTERN_STR.end();

    std::string::const_iterator pos = std::find_first_of(begin, end, pb, pe);
    if (end == pos) {
        return str;
    }

    std::string dest;
    std::size_t to_append = pos - begin;
    std::size_t left = str.size() - to_append;
    dest.reserve(str.size() + left / 4);

    dest.append(str.c_str(), to_append);
    escape(Range(str.c_str() + to_append, str.c_str() + str.size()), dest);
    return dest;
}

const char*
XmlUtils::findScriptCode(xmlNodePtr node) {

    const char *code = XmlUtils::cdataValue(node);
    if (NULL != code) {
        for (const char *p = code; *p; ++p) {
            if (!::isspace(*p)) {
                return code;
            }
        }
        return NULL;
    }

    for (xmlNodePtr child = node->children; NULL != child; child = child->next) {
        code = (const char*) child->content;
        if (NULL != code && XML_TEXT_NODE == child->type) {
            for (const char *p = code; *p; ++p) {
                if (!::isspace(*p)) {
                    return code;
                }
            }
        }
    }
    return NULL;
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
    std::map<std::string, std::string> ns;
    return xpathNsExists(doc, path, ns);
}

std::string
XmlUtils::xpathValue(xmlDocPtr doc, const std::string &path, const std::string &defval) {
    std::map<std::string, std::string> ns;
    return xpathNsValue(doc, path, ns, defval);
}

bool
XmlUtils::xpathNsExists(xmlDocPtr doc, const std::string &path,
    const std::map<std::string, std::string> &ns) {

    XmlXPathContextHelper ctx(xmlXPathNewContext(doc));
    throwUnless(NULL != ctx.get());

    XmlUtils::regiserNsList(ctx.get(), ns);

    XmlXPathObjectHelper object(xmlXPathEvalExpression((const xmlChar*) path.c_str(), ctx.get()));
    throwUnless(NULL != object.get());
    return (NULL != object->nodesetval && 0 != object->nodesetval->nodeNr);
}

std::string
XmlUtils::xpathNsValue(xmlDocPtr doc, const std::string &path,
    const std::map<std::string, std::string> &ns, const std::string &defval) {

    std::string res = defval;
    XmlXPathContextHelper ctx(xmlXPathNewContext(doc));
    throwUnless(NULL != ctx.get());

    XmlUtils::regiserNsList(ctx.get(), ns);

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
    const Request *request = VirtualHostData::instance()->get();
    if (NULL == request) {
        return StringUtils::EMPTY_STRING;
    }
    return request->getOriginalUrl();
}

static void
patchErrorInfo(const std::string &fileName, std::string error, const std::string &url, const char *id) {

    if (!url.empty()) {
        error.append(" URL: ", 6).append(url);
    }
    if (NULL != id && '\0' != *id) {
        error.append(" ID: ", 5).append(id);
    }
    error.append(" Request URL: ").append(getOriginalUrl());
    log()->error("%s", error.c_str());

    XmlInfoCollector::ErrorMapType *error_info = XmlInfoCollector::getErrorInfo();
    if (NULL != error_info) {
        if (!fileName.empty()) {
            error_info->insert(std::make_pair(fileName, error));
        }
        else if (!url.empty()) {
            error_info->insert(std::make_pair(url, error));
        }
    }
}

static void
patchModifiedInfoUnexisted(const std::string &fileName) {

    if (fileName.empty()) {
        return;
    }
    Xml::TimeMapType *modified_info = XmlInfoCollector::getModifiedInfo();
    if (NULL == modified_info) {
        return;
    }
    boost::filesystem::path path(fileName);
    modified_info->insert(std::make_pair(path.native_file_string(), 0));
}

static void
patchModifiedInfo(const std::string &fileName, const std::string &url, const char *id) {

    Xml::TimeMapType *modified_info = XmlInfoCollector::getModifiedInfo();
    if (NULL == modified_info) {
        return;
    }

    if (fileName.find("://") == std::string::npos) {
        namespace fs = boost::filesystem;
        try {
            fs::path path(fileName);
            if (fs::exists(path) && !fs::is_directory(path)) {
                time_t modified = fs::last_write_time(path);
                modified_info->insert(std::make_pair(path.native_file_string(), modified));
            }
        }
        catch (const fs::filesystem_error &e) {
            patchErrorInfo(fileName, STR_ERROR_CANNOT_STAT_FILE + e.what(), url, id);
        }
    }
}

xmlParserInputPtr
XmlUtils::entityResolver(const char *url, const char *id, xmlParserCtxtPtr ctxt) {
    if (NULL == url) {
        url = "";
    }
    if (NULL == id) {
        id = "";
    }
    if (!*url && !*id) {
        return NULL;
    }

    xmlParserInputPtr ret = NULL;
    try {
        Logger *l = log();
        if (l->enabledDebug()) l->debug("entityResolver. url: %s, id: %s", url, id);

        if ('\0' != *url && Policy::isRemoteScheme(url)) {
            if (l->enabledWarn()) {
                l->warn("entityResolver: loading remote entity is forbidden. URL: %s ID: %s Request URL: %s",
                    url, id, getOriginalUrl().c_str());
            }
            return NULL;
        }

        std::string fileName;
        std::string url_str = url;
        if (!url_str.empty()) {
            if ('/' == *url) {
                fileName.assign(url_str);
            }
            else {
                fileName = Policy::instance()->getPathByScheme(NULL, url_str);
                if (fileName.empty()) {
                    return NULL;
                }
                if (l->enabledInfo() && fileName != url_str) {
                    l->info("entityResolver: url %s changed to %s", url, fileName.c_str());
                }
            }
        }
        try {
            XmlErrorReporter *reporter = XmlErrorReporter::reporter();
            bool had_error = reporter->hasError();
            ret = external_entity_default_loader_(fileName.c_str(), id, ctxt);
            if (!had_error && reporter->hasError()) {
                const std::string &message = reporter->message();
                if (std::string::npos != message.find(STR_ENTITY_RESOLVER_LOADING_FAILED) &&
                    std::string::npos == message.find(STR_ENTITY_RESOLVER_NO_FALLBACK)) {
                    reporter->reset();
                }
            }
            if (NULL != ret) {
                patchModifiedInfo(fileName, url_str, id);
                return ret;
            }
            patchModifiedInfoUnexisted(fileName);
            if (l->enabledWarn()) {
                std::string error;
                if ('\0' != *id) {
                    error.append(" ID: ", 5).append(id);
                }
                l->warn("Cannot resolve external entity: %s%s Request URL: %s", url, error.c_str(), getOriginalUrl().c_str());
            }
        }
        catch (const std::exception &e) {
            patchErrorInfo(fileName, STR_ENTITY_RESOLVER_ERROR + e.what(), url_str, id);
        }
        catch (...) {
            patchErrorInfo(fileName, STR_ENTITY_RESOLVER_UNKNOWN_ERROR, url_str, id);
        }
    }
    catch (const std::exception &e) {
        log()->error("entityResolver error: %s URL: %s ID: %s Request URL: %s", e.what(), url, id, getOriginalUrl().c_str());
    }
    catch (...) {
        log()->error("entityResolver unknown error. URL: %s ID: %s Request URL: %s", url, id, getOriginalUrl().c_str());
    }
    return ret;
}

xmlParserInputBufferPtr
XmlUtilsFileResolver(const char *uri, xmlCharEncoding enc) {

    if (NULL == uri || !*uri) {
        return old_libxml_file_resolver_(uri, enc);
    }
    xmlParserInputBufferPtr ret = NULL;
    try {
        Logger *l = log();
        if (l->enabledDebug()) l->debug("FileResolver: URI: %s", uri);

        if (Policy::isRemoteScheme(uri)) {
            if (l->enabledWarn()) {
                log()->warn("FileResolver: loading remote file is forbidden. URL: %s Request URL: %s",
                    uri, getOriginalUrl().c_str());
            }
            return NULL;
        }

        std::string url_str = uri;
        std::string fileName;
        if ('/' == *uri) {
            fileName.assign(url_str);
        }
        else {
            fileName = Policy::instance()->getPathByScheme(NULL, url_str);
            if (fileName.empty()) {
                return NULL;
            }
            if (l->enabledInfo() && fileName != url_str) {
                l->info("FileResolver: url %s changed to %s", uri, fileName.c_str());
            }
        }

        ret = old_libxml_file_resolver_(fileName.c_str(), enc);
        if (NULL != ret) {
            patchModifiedInfo(fileName, url_str, NULL);
        }
        else {
            patchModifiedInfoUnexisted(fileName);
        }
    }
    catch (const std::exception &e) {
        log()->error("FileResolver error: %s URL: %s Request URL: %s", e.what(), uri, getOriginalUrl().c_str());
    }
    catch (...) {
        log()->error("FileResolver unknown error. URL: %s Request URL: %s", uri, getOriginalUrl().c_str());
    }
    return ret;
}

xmlDocPtr
XmlUtils::fakeXml() {
    return xml_fake_doc_.get();
}

std::string
XmlUtils::getUniqueNodeId(xmlNodePtr node) {
    std::string id;
    xmlNodePtr parent = node;
    do {
        xmlNodePtr prev = parent->prev;
        int node_pos = 0;
        while(prev) {
            ++node_pos;
            prev = prev->prev;
        }
        id.append(boost::lexical_cast<std::string>(node_pos));
        parent = parent->parent;
    }
    while(parent);
    return id;
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

void
XmlUtils::regiserNsList(xmlXPathContextPtr ctx, const std::map<std::string, std::string> &ns) {
    for(std::map<std::string, std::string>::const_iterator it_ns = ns.begin();
        it_ns != ns.end();
        ++it_ns) {
        xmlXPathRegisterNs(ctx, (const xmlChar *)it_ns->first.c_str(),
            (const xmlChar *)it_ns->second.c_str());
    }
}

XmlTypedVisitor::XmlTypedVisitor()
{}

XmlTypedVisitor::~XmlTypedVisitor()
{}

void
XmlTypedVisitor::visitNil() {
    appendResult(createNode(TypedValue::TYPE_NIL_STRING.c_str(), NULL));
}

void
XmlTypedVisitor::visitBool(bool value) {
    appendResult(createNode(TypedValue::TYPE_BOOL_STRING.c_str(),
        boost::lexical_cast<std::string>(value).c_str()));
}
void
XmlTypedVisitor::visitInt32(boost::int32_t value) {
    appendResult(createNode(TypedValue::TYPE_LONG_STRING.c_str(),
        boost::lexical_cast<std::string>(value).c_str()));
}

void
XmlTypedVisitor::visitUInt32(boost::uint32_t value) {
    appendResult(createNode(TypedValue::TYPE_ULONG_STRING.c_str(),
        boost::lexical_cast<std::string>(value).c_str()));
}

void
XmlTypedVisitor::visitInt64(boost::int64_t value) {
    appendResult(createNode(TypedValue::TYPE_LONGLONG_STRING.c_str(),
        boost::lexical_cast<std::string>(value).c_str()));
}

void
XmlTypedVisitor::visitUInt64(boost::uint64_t value) {
    appendResult(createNode(TypedValue::TYPE_ULONGLONG_STRING.c_str(),
        boost::lexical_cast<std::string>(value).c_str()));
}

void
XmlTypedVisitor::visitDouble(double value) {
    appendResult(createNode(TypedValue::TYPE_DOUBLE_STRING.c_str(),
        boost::lexical_cast<std::string>(value).c_str()));
}

void
XmlTypedVisitor::visitString(const std::string &value) {
    appendResult(createNode(TypedValue::TYPE_STRING_STRING.c_str(), value.c_str()));
}

XmlNodeHelper
XmlTypedVisitor::createNode(const char *type, const char *value) {
    XmlNodeHelper node(xmlNewNode(NULL, (const xmlChar*)"param"));
    xmlNewProp(node.get(), (const xmlChar*)"type", (const xmlChar*)type);
    if (value) {
        xmlNodeSetContent(node.get(), (const xmlChar*)XmlUtils::escape(value).c_str());
    }
    return node;
}

void
XmlTypedVisitor::appendResult(XmlNodeHelper result) {
    if (result_.get()) {
        xmlAddChild(result_.get(), result.release());
    }
    else {
        result_ = result;
    }
}

void
XmlTypedVisitor::visitArray(const TypedValue::ArrayType &value) {
    XmlNodeHelper result_old = result_;
    XmlNodeHelper root = createNode("Array", NULL);
    for (TypedValue::ArrayType::const_iterator it = value.begin();
         it != value.end();
         ++it) {
        it->visit(this);
        xmlAddChild(root.get(), result_.release());
    }
    result_ = result_old;
    appendResult(root);
}

void
XmlTypedVisitor::visitMap(const TypedValue::MapType &value) {
    XmlNodeHelper result_old = result_;
    XmlNodeHelper root = createNode("Map", NULL);
    for (TypedValue::MapType::const_iterator it = value.begin();
         it != value.end();
         ++it) {
        it->second.visit(this);
        xmlNewProp(result_.get(), (const xmlChar*)"name", (const xmlChar*)it->first.c_str());
        xmlAddChild(root.get(), result_.release());
    }
    result_ = result_old;
    appendResult(root);
}

XmlNodeHelper
XmlTypedVisitor::result() const {
    return result_;
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
    if (message_.size() >= MESSAGE_SIZE_LIMIT) {
        return;
    }
    char part_message[CHUNK_MESSAGE_SIZE];
    vsnprintf(part_message, sizeof(part_message), format, args);
    part_message[CHUNK_MESSAGE_SIZE - 1] = 0;
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
