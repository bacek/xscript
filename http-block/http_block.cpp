#include "settings.h"

#include <algorithm>
#include <cassert>
#include <sstream>
#include <stdexcept>

#include <boost/tokenizer.hpp>
#include <boost/current_function.hpp>
#include <boost/checked_delete.hpp>

#include <libxml/HTMLparser.h>

#include "http_block.h"

#include "xscript/args.h"
#include "xscript/context.h"
#include "xscript/http_helper.h"
#include "xscript/logger.h"
#include "xscript/meta.h"
#include "xscript/operation_mode.h"
#include "xscript/param.h"
#include "xscript/policy.h"
#include "xscript/profiler.h"
#include "xscript/request.h"
#include "xscript/state.h"
#include "xscript/string_utils.h"
#include "xscript/util.h"
#include "xscript/writer.h"
#include "xscript/xml.h"
#include "xscript/xml_util.h"

#include "internal/hash.h"
#include "internal/hashmap.h"

#ifndef HAVE_HASHMAP
#include <map>
#endif

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif


namespace xscript {

#ifndef HAVE_HASHMAP
typedef std::map<std::string, HttpMethod> MethodMap;
#else
typedef details::hash_map<std::string, HttpMethod, details::StringHash> MethodMap;
#endif


class HttpMethodRegistrator {
public:
    HttpMethodRegistrator();
};

class StringBinaryWriter : public BinaryWriter {
public:
    StringBinaryWriter(boost::shared_ptr<std::string> data) : data_(data) {}

    void write(std::ostream *os, const Response *response) const {
        (void)response;
        os->write(data_->data(), data_->size());
    }

    std::streamsize size() const {
        return data_->size();
    }
    
    virtual ~StringBinaryWriter() {}
private:
    boost::shared_ptr<std::string> data_;
};

static MethodMap methods_;
static std::string STR_HEADERS("headers");

HttpBlock::HttpBlock(const Extension *ext, Xml *owner, xmlNodePtr node) :
        Block(ext, owner, node), RemoteTaggedBlock(ext, owner, node),
        method_(NULL), proxy_(false), print_error_(false) {
}

HttpBlock::~HttpBlock() {
    std::for_each(headers_.begin(), headers_.end(), boost::checked_deleter<Param>());
}

void
HttpBlock::parseSubNode(xmlNodePtr node) {

    if (NULL == node->name || 0 != xmlStrcasecmp(node->name, (const xmlChar*)"header")) {
        Block::parseSubNode(node);
        return;
    }

    const xmlChar* ref = node->ns ? node->ns->href : NULL;
    if (NULL != ref && 0 != xmlStrcasecmp(ref, (const xmlChar*)XmlUtils::XSCRIPT_NAMESPACE)) {
        Block::parseSubNode(node);
        return;
    }

    std::auto_ptr<Param> p = createParam(node);
    const std::string &id = p->id();
    log()->debug("creating header param %s in http-block: %s", id.c_str(), owner()->name().c_str());
    if (id.empty()) {
        throw std::runtime_error("header param without id");
    }

    int size = id.size();
    if (size > 128) {
        throw UnboundRuntimeError(std::string("header param with too big size id: ") + id);
    }

    if (!isalpha(id[0])) {
        throw std::runtime_error(std::string("header param with incorrect first character in id: ") + id);
    }

    for (int i = 1; i < size; ++i) {
        char character = id[i];
        if (isalnum(character) || character == '-') {
            continue;
        }

        std::stringstream ss;
        ss << "header param with incorrect character at " << i + 1 << " in id: " << id;
        throw std::runtime_error(ss.str());
    }

    header_names_.insert(id);
    headers_.push_back(p.get());
    p.release();
}

void
HttpBlock::postParse() {

    if (proxy_ && tagged()) {
        log()->warn("swithch off tagging in proxy http-block: %s", owner()->name().c_str());
        tagged(false);
    }

    RemoteTaggedBlock::postParse();

    createCanonicalMethod("http.");

    MethodMap::iterator i = methods_.find(method());
    if (methods_.end() != i) {
        method_ = i->second;
    }
    else {
        std::stringstream stream;
        stream << "nonexistent http method call: " << method();
        throw std::invalid_argument(stream.str());
    }
}

void
HttpBlock::retryCall(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const throw (std::exception) {
    invoke_ctx->resultDoc((this->*method_)(ctx.get(), invoke_ctx.get()));
}

void
HttpBlock::property(const char *name, const char *value) {

    if (strncasecmp(name, "proxy", sizeof("proxy")) == 0) {
        proxy_ = (strncasecmp(value, "yes", sizeof("yes")) == 0);
    }
    else if (strncasecmp(name, "encoding", sizeof("encoding")) == 0) {
        charset_ = value;
    }
    else if (strncasecmp(name, "print-error-body", sizeof("print-error-body")) == 0) {
        print_error_ = (strncasecmp(value, "yes", sizeof("yes")) == 0);
    }
    else {
        RemoteTaggedBlock::property(name, value);
    }
}

std::string
HttpBlock::info(const Context *ctx) const {

    if (headers_.empty()) {
        return RemoteTaggedBlock::info(ctx);
    }

    std::string info = RemoteTaggedBlock::info(ctx);
    info.append(" | Headers: ");
    for (unsigned int i = 0, n = headers_.size(); i < n; ++i) {
        if (i > 0) {
            info.append(", ");
        }
        const Param *param = headers_[i];
        info.append(param->type());
        const std::string& value =  param->value();
        if (!value.empty()) {
            info.push_back('(');
            info.append(value);
            info.push_back(')');
        }
    }
    return info;
}

std::string
HttpBlock::createTagKey(const Context *ctx, const InvokeContext *invoke_ctx) const {

    if (headers_.empty()) {
        return RemoteTaggedBlock::createTagKey(ctx, invoke_ctx);
    }

    std::string key = RemoteTaggedBlock::createTagKey(ctx, invoke_ctx);
    key.append("| Headers:");
    const ArgList *args = invoke_ctx->getExtraArgList(STR_HEADERS);
    assert(args);
    assert(args->size() == headers_.size());
    key.append(paramsKey(args));
    return key;
}

ArgList*
HttpBlock::createArgList(Context *ctx, InvokeContext *invoke_ctx) const {

    if (!headers_.empty()) {
        boost::shared_ptr<CommonArgList> args(new CommonArgList());
        for (std::vector<Param*>::const_iterator it = headers_.begin(), end = headers_.end(); it != end; ++it) {
            (*it)->add(ctx, *args);
        }
        invoke_ctx->setExtraArgList(STR_HEADERS, args);
    }
    return RemoteTaggedBlock::createArgList(ctx, invoke_ctx);
}

std::string
HttpBlock::getUrl(const ArgList *args, unsigned int first, unsigned int last) const {
    std::string url = Block::concatArguments(args, first, last);
    if (strncasecmp(url.c_str(), "file://", sizeof("file://") - 1) == 0) {
        throw InvokeError("File scheme is not allowed", "url", url);
    }
    return url;
    
}

XmlDocHelper
HttpBlock::getHttp(Context *ctx, InvokeContext *invoke_ctx) const {
    log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());

    const ArgList* args = invoke_ctx->getArgList();
    unsigned int size = args->size();
    if (!size) {
        throwBadArityError();
    }
    
    std::string url = getUrl(args, 0, size - 1);
    PROFILER(log(), "getHttp: " + url);

    HttpHelper helper(url, getTimeout(ctx, url));
    
    appendHeaders(helper, ctx->request(), invoke_ctx, true);
    httpCall(helper);
    checkStatus(helper);
    
    createTagInfo(helper, invoke_ctx);

    if (invoke_ctx->haveCachedCopy() && !invoke_ctx->tag().modified) {
        return XmlDocHelper();
    }

    createMeta(helper, invoke_ctx);

    return response(helper);
}


XmlDocHelper
HttpBlock::getBinaryPage(Context *ctx, InvokeContext *invoke_ctx) const {
    log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());

    const ArgList* args = invoke_ctx->getArgList();
    unsigned int size = args->size();
    if (!size) {
        throwBadArityError();
    }
    else if (tagged()) {
        throw CriticalInvokeError("tag is not allowed");
    }

    std::string url = getUrl(args, 0, size - 1);
    PROFILER(log(), "getBinaryPage: " + url);
    
    HttpHelper helper(url, getTimeout(ctx, url));
    
    appendHeaders(helper, ctx->request(), invoke_ctx, false);
    httpCall(helper);

    if (!helper.isOk()) {
        long status = helper.status();
        RetryInvokeError error("Incorrect http status", "url", url);
        error.add("status", boost::lexical_cast<std::string>(status));
        throw error;
    }

    createMeta(helper, invoke_ctx);

    ctx->response()->write(
        std::auto_ptr<BinaryWriter>(new StringBinaryWriter(helper.content())));
    
    XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != doc.get());
    
    XmlNodeHelper node(xmlNewDocNode(doc.get(), NULL, (const xmlChar*)"success", (const xmlChar*)"1"));
    XmlUtils::throwUnless(NULL != node.get());
    
    const std::string& content_type = helper.contentType();
    if (!content_type.empty()) {
        xmlNewProp(node.get(), (const xmlChar*)"content-type", (const xmlChar*)XmlUtils::escape(content_type).c_str());
        ctx->response()->setHeader("Content-type", content_type);
    }        
    xmlNewProp(node.get(), (const xmlChar*)"url", (const xmlChar*)XmlUtils::escape(url).c_str());
    xmlDocSetRootElement(doc.get(), node.release());

    return doc;
}


XmlDocHelper
HttpBlock::postHttp(Context *ctx, InvokeContext *invoke_ctx) const {

    log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());

    const ArgList* args = invoke_ctx->getArgList();
    unsigned int size = args->size();
    if (size < 2) {
        throwBadArityError();
    }

    std::string url = getUrl(args, 0, size - 2);
    PROFILER(log(), "postHttp: " + url);
    
    HttpHelper helper(url, getTimeout(ctx, url));
    
    appendHeaders(helper, ctx->request(), invoke_ctx, true);

    const std::string& body = args->at(size-1);
    helper.postData(body.data(), body.size());

    httpCall(helper);
    checkStatus(helper);

    createTagInfo(helper, invoke_ctx);

    if (invoke_ctx->haveCachedCopy() && !invoke_ctx->tag().modified) {
        return XmlDocHelper();
    }

    createMeta(helper, invoke_ctx);

    return response(helper);
}

XmlDocHelper
HttpBlock::postByRequest(Context *ctx, InvokeContext *invoke_ctx) const {
    (void)invoke_ctx;
    
    log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());

    const ArgList* args = invoke_ctx->getArgList();
    unsigned int size = args->size();
    if (!size) {
        throwBadArityError();
    }
    else if (tagged()) {
        throw CriticalInvokeError("tag is not allowed");
    }

    std::string url = getUrl(args, 0, size - 1);
    PROFILER(log(), "postByRequest: " + url);
    
    bool is_get = (strcmp(ctx->request()->getRequestMethod().c_str(), "GET") == 0);
    
    if (!is_get) {
        const std::string &query = ctx->request()->getQueryString();
        if (!query.empty()) {
            url.push_back(url.find('?') != std::string::npos ? '&' : '?');
            url.append(query);
        }
    }
    
    HttpHelper helper(url, getTimeout(ctx, url));
    appendHeaders(helper, ctx->request(), invoke_ctx, false);

    if (is_get) {
        const std::string &query = ctx->request()->getQueryString();
        helper.postData(query.c_str(), query.length());
    }
    else {
        std::pair<const char*, std::streamsize> body = ctx->request()->requestBody();
        helper.postData(body.first, body.second);
    }

    httpCall(helper);
    checkStatus(helper);
    createMeta(helper, invoke_ctx);
    
    return response(helper);
}

XmlDocHelper
HttpBlock::getByState(Context *ctx, InvokeContext *invoke_ctx) const {
    (void)invoke_ctx;

    log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());
    
    const ArgList* args = invoke_ctx->getArgList();
    unsigned int size = args->size();
    if (!size) {
        throwBadArityError();
    }
    else if (tagged()) {
        throw CriticalInvokeError("tag is not allowed");
    }

    std::string url = getUrl(args, 0, size - 1);
    PROFILER(log(), "getByState: " + url);
    
    bool has_query = url.find('?') != std::string::npos;

    State* state = ctx->state();
    std::vector<std::string> names;
    state->keys(names);

    for (std::vector<std::string>::const_iterator i = names.begin(), end = names.end(); i != end; ++i) {
        const std::string &name = *i;
        url.append(1, has_query ? '&' : '?');
        url.append(name);
        url.append(1, '=');
        url.append(state->asString(name));
        has_query = true;
    }
    
    HttpHelper helper(url, getTimeout(ctx, url));
    
    appendHeaders(helper, ctx->request(), invoke_ctx, false);
    httpCall(helper);
    checkStatus(helper);
    createMeta(helper, invoke_ctx);
    
    return response(helper);
}

XmlDocHelper
HttpBlock::getByRequest(Context *ctx, InvokeContext *invoke_ctx) const {
    (void)invoke_ctx;

    log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());

    const ArgList* args = invoke_ctx->getArgList();
    unsigned int size = args->size();
    if (!size) {
        throwBadArityError();
    }
    else if (tagged()) {
        throw CriticalInvokeError("tag is not allowed");
    }

    std::string url = getUrl(args, 0, size - 1);
    PROFILER(log(), "getByRequest: " + url);
    
    bool is_post = (strcmp(ctx->request()->getRequestMethod().c_str(), "POST") == 0);
    
    if (!is_post) {
        const std::string &query = ctx->request()->getQueryString();
        if (!query.empty()) {
            url.push_back(url.find('?') != std::string::npos ? '&' : '?');
            url.append(query);
        }
    }
    else {
        const std::vector<StringUtils::NamedValue>& args = ctx->request()->args();
        if (!args.empty()) {
            bool has_query = url.find('?') != std::string::npos;
            for(std::vector<StringUtils::NamedValue>::const_iterator it = args.begin(), end = args.end();
                it != end;
                ++it) {
                url.push_back(has_query ? '&' : '?');
                url.append(it->first);
                url.push_back('=');
                url.append(it->second);
                has_query = true;
            }
        }
    }
    
    HttpHelper helper(url, getTimeout(ctx, url));
    
    appendHeaders(helper, ctx->request(), invoke_ctx, false);
    httpCall(helper);
    checkStatus(helper);
    createMeta(helper, invoke_ctx);
    
    return response(helper);
}

void
HttpBlock::appendHeaders(HttpHelper &helper, const Request *request, const InvokeContext *invoke_ctx, bool allow_tag) const {
    std::vector<std::string> headers;
    bool real_ip = false;
    const std::string &ip_header_name = Policy::instance()->realIPHeaderName();
    if (proxy_ && request->countHeaders() > 0) {    
        std::vector<std::string> names;
        request->headerNames(names);
        for (std::vector<std::string>::const_iterator i = names.begin(), end = names.end(); i != end; ++i) {
            const std::string &name = *i;
            if (name.empty()) {
                continue;
            }
            const std::string &value = request->getHeader(name);
            if (invoke_ctx && header_names_.end() != header_names_.find(name)) {
                log()->debug("proxy header was skipped (override) %s: %s", name.c_str(), value.c_str());
            }
            else if (Policy::instance()->isSkippedProxyHeader(name)) {
                log()->debug("proxy header was skipped (policy) %s: %s", name.c_str(), value.c_str());
            }
            else {
                if (!real_ip && strcasecmp(ip_header_name.c_str(), name.c_str()) == 0) {
                    real_ip = true;
                }
                headers.push_back(name);
                headers.back().append(": ").append(value);
            }
        }
    }

    if (invoke_ctx && !headers_.empty()) {
        const ArgList *args = invoke_ctx->getExtraArgList(STR_HEADERS);
        assert(NULL != args);
        unsigned int sz = args->size();
        assert(sz == headers_.size());
        for (unsigned int i = 0; i != sz; ++i) {
            const Param *param = headers_[i];
            const std::string &name = param->id();
            if (!real_ip && strcasecmp(ip_header_name.c_str(), name.c_str()) == 0) {
                real_ip = true;
            }
            headers.push_back(name);
            headers.back().append(": ").append(args->at(i));
        }
    }

    if (!real_ip && !ip_header_name.empty()) {
        headers.push_back(ip_header_name);
        headers.back().append(": ").append(request->getRealIP());
    }
    if (allow_tag && invoke_ctx && invoke_ctx->tagged()) {
        helper.appendHeaders(headers, invoke_ctx->tag().last_modified);
    }
    else {
        helper.appendHeaders(headers, Tag::UNDEFINED_TIME);
    }
}

XmlDocHelper
HttpBlock::response(const HttpHelper &helper, bool error_mode) const {
    XmlDocHelper result;
    boost::shared_ptr<std::string> str = helper.content();
    if (helper.isXml()) {
        XmlDocHelper result(xmlReadMemory(str->c_str(), str->size(), "",
            charset_.empty() ? NULL : charset_.c_str(), XML_PARSE_DTDATTR | XML_PARSE_NOENT));
        XmlUtils::throwUnless(NULL != result.get(), "Url", helper.url().c_str());
        OperationMode::instance()->processXmlError(helper.url());
        return result;
    }
    
    if (!error_mode && helper.contentType() == "text/html") {
        std::string data = XmlUtils::sanitize(*str, StringUtils::EMPTY_STRING, 0);
        if (data.empty()) {
            throw InvokeError("Empty sanitized text/html document");
        }

        XmlDocHelper result(xmlReadMemory(data.c_str(), data.size(), helper.base().c_str(),
                helper.charset().c_str(), XML_PARSE_DTDATTR | XML_PARSE_NOENT));

        if (NULL == result.get()) {
            log()->error("Invalid sanitized text/html document. Url: %s", helper.url().c_str());
            std::string error = "Invalid sanitized text/html document. Url: " + helper.url() + ". Error: ";
            std::string xml_error = XmlUtils::getXMLError();
            xml_error.empty() ? error.append("Unknown XML error") : error.append(xml_error);
            OperationMode::instance()->processCriticalInvokeError(error);
        }
        
        OperationMode::instance()->processXmlError(helper.url());
        
        return result;
    }
    
    if (0 == strncmp(helper.contentType().c_str(), "text/", sizeof("text/") - 1)) {
        if (str->empty()) {
            XmlDocHelper result(xmlNewDoc((const xmlChar*) "1.0"));
            XmlUtils::throwUnless(NULL != result.get());
            XmlNodeHelper node(xmlNewDocNode(result.get(), NULL, (const xmlChar*)"text", NULL));
            XmlUtils::throwUnless(NULL != node.get());
            xmlDocSetRootElement(result.get(), node.release());
            return result;
        }

        std::string res;
        res.append("<text>").append(XmlUtils::escape(*str)).append("</text>");
        XmlDocHelper result(xmlReadMemory(res.c_str(), res.size(), "",
                helper.charset().c_str(), XML_PARSE_DTDATTR | XML_PARSE_NOENT));
        XmlUtils::throwUnless(NULL != result.get(), "Url", helper.url().c_str());
        OperationMode::instance()->processXmlError(helper.url());
        return result;
    }

    if (error_mode) {
        return XmlDocHelper();
    }
        
    throw InvokeError("format is not recognized: " + helper.contentType(), "url", helper.url());
}

void
HttpBlock::createTagInfo(const HttpHelper &helper, InvokeContext *invoke_ctx) const {
    invoke_ctx->resetTag();
    if (!tagged()) {
        return;
    }
    invoke_ctx->tag(helper.createTag());
}

int
HttpBlock::getTimeout(Context *ctx, const std::string &url) const {
    int timeout = remainedTime(ctx);
    if (timeout > 0) {
        return timeout;
    }
    
    InvokeError error("block is timed out");
    error.add("url", url);
    error.add("timeout", boost::lexical_cast<std::string>(ctx->timer().timeout()));
    throw error;
}

void
HttpBlock::wrapError(InvokeError &error, const HttpHelper &helper, const XmlNodeHelper &error_body_node) const {
    error.add("url", helper.url());
    error.add("status", boost::lexical_cast<std::string>(helper.status()));
    if (!helper.contentType().empty()) {
        error.add("content-type", helper.contentType());
    }
    if (NULL != error_body_node.get()) {
        error.attachNode(error_body_node);
    }
}

void
HttpBlock::checkStatus(const HttpHelper &helper) const {
    try {
        helper.checkStatus();
    }
    catch(InvokeError &e) {
        wrapError(e, helper, XmlNodeHelper());
        throw;
    }
    catch(const std::runtime_error &e) {
        XmlNodeHelper error_body_node;
        if (print_error_ && 404 != helper.status() && helper.content()->size() > 0) {
            XmlDocHelper doc = response(helper, true);
            if (NULL != doc.get()) {
                xmlNodePtr root_node = xmlDocGetRootElement(doc.get());
                if (root_node) {
                    error_body_node = XmlNodeHelper(xmlCopyNode(root_node, 1));
                }
            }
        }
        if (helper.status() >= 500) {
            RetryInvokeError error(e.what());
            wrapError(error, helper, error_body_node);
            throw error;
        }
        else {
            InvokeError error(e.what());
            wrapError(error, helper, error_body_node);
            throw error;
        }
    }
}

void
HttpBlock::httpCall(HttpHelper &helper) const {
    try {
        helper.perform();
    }
    catch(const std::runtime_error &e) {
        throw RetryInvokeError(e.what(), "url", helper.url());
    }
    log()->debug("%s, http call performed", BOOST_CURRENT_FUNCTION);
}

void
HttpBlock::createMeta(HttpHelper &helper, InvokeContext *invoke_ctx) const {
    if (metaBlock()) {
        typedef std::multimap<std::string, std::string> HttpHeaderMap;
        typedef HttpHeaderMap::const_iterator HttpHeaderIter;
        const HttpHeaderMap& headers = helper.headers();
        for (HttpHeaderIter it = headers.begin(); it != headers.end(); ) {
            std::pair<HttpHeaderIter, HttpHeaderIter> res = headers.equal_range(it->first);
            HttpHeaderIter itr = res.first;
            std::vector<std::string> result;
            std::string name = "HTTP_" + StringUtils::toupper(it->first);
            for (; itr != res.second; ++itr) {
                result.push_back(itr->second);
            }
            result.size() == 1 ?
                invoke_ctx->meta()->setString(name, result[0]) :
                invoke_ctx->meta()->setArray(name, result);
            it = itr;
        }
    }
}

void
HttpBlock::registerMethod(const char *name, HttpMethod method) {
    try {
        std::string n(name);
        std::pair<std::string, HttpMethod> p(n, method);

        MethodMap::iterator i = methods_.find(n);
        if (methods_.end() == i) {
            methods_.insert(p);
        }
        else {
            std::stringstream stream;
            stream << "registering duplicate http method: " << n;
            throw std::invalid_argument(stream.str());
        }
    }
    catch (const std::exception &e) {
        xscript::log()->error("%s, caught exception: %s", BOOST_CURRENT_FUNCTION, e.what());
        throw;
    }
}

HttpMethodRegistrator::HttpMethodRegistrator() {
    HttpBlock::registerMethod("getHttp", &HttpBlock::getHttp);
    HttpBlock::registerMethod("get_http", &HttpBlock::getHttp);

    HttpBlock::registerMethod("getHTTP", &HttpBlock::getHttp);
    HttpBlock::registerMethod("getPageT", &HttpBlock::getHttp);
    HttpBlock::registerMethod("curlGetHttp", &HttpBlock::getHttp);

    HttpBlock::registerMethod("postHttp", &HttpBlock::postHttp);
    HttpBlock::registerMethod("post_http", &HttpBlock::postHttp);
    HttpBlock::registerMethod("postHTTP", &HttpBlock::postHttp);

    HttpBlock::registerMethod("getByState", &HttpBlock::getByState);
    HttpBlock::registerMethod("get_by_state", &HttpBlock::getByState);

    HttpBlock::registerMethod("getByRequest", &HttpBlock::getByRequest);
    HttpBlock::registerMethod("get_by_request", &HttpBlock::getByRequest);
    
    HttpBlock::registerMethod("postByRequest", &HttpBlock::postByRequest);
    HttpBlock::registerMethod("post_by_request", &HttpBlock::postByRequest);

    HttpBlock::registerMethod("getBinaryPage", &HttpBlock::getBinaryPage);
    HttpBlock::registerMethod("get_binary_page", &HttpBlock::getBinaryPage);
}

static HttpMethodRegistrator reg_;

} // namespace xscript
