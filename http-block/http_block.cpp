#include "settings.h"

#include <sys/stat.h>

#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

#include <boost/tokenizer.hpp>
#include <boost/current_function.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <libxml/HTMLparser.h>

#include "http_block.h"

#include "xscript/context.h"
#include "xscript/http_helper.h"
#include "xscript/logger.h"
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

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class HttpMethodRegistrator {
public:
    HttpMethodRegistrator();
};

class StringBinaryWriter : public BinaryWriter {
public:
    StringBinaryWriter(boost::shared_ptr<std::string> data) : data_(data) {}

    void write(std::ostream *os) const {
        os->write(data_->data(), data_->size());
    }

    std::streamsize size() const {
        return data_->size();
    }
    
    virtual ~StringBinaryWriter() {}
private:
    boost::shared_ptr<std::string> data_;
};

MethodMap HttpBlock::methods_;

HttpBlock::HttpBlock(const Extension *ext, Xml *owner, xmlNodePtr node) :
        Block(ext, owner, node), RemoteTaggedBlock(ext, owner, node),
        proxy_(false), print_error_(false), method_(NULL) {
}

HttpBlock::~HttpBlock() {
}

void
HttpBlock::postParse() {

    if (proxy_ && tagged()) {
        log()->warn("%s, proxy in tagged http-block: %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());
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

XmlDocHelper
HttpBlock::retryCall(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) throw (std::exception) {
    return (this->*method_)(ctx.get(), invoke_ctx.get());
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
HttpBlock::concatParams(const Context *ctx, unsigned int first, unsigned int last) const {  
    std::string url = Block::concatParams(ctx, first, last);
    if (strncasecmp(url.c_str(), "file://", sizeof("file://") - 1) == 0) {
        throw InvokeError("File scheme is not allowed", "url", url);
    }
    return url;
    
}

XmlDocHelper
HttpBlock::getHttp(Context *ctx, InvokeContext *invoke_ctx) {

    log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());

    const std::vector<Param*> &p = params();
    unsigned int size = p.size();
    if (size == 0) {
        throwBadArityError();
    }
    
    std::string url = concatParams(ctx, 0, size - 1);
    
    PROFILER(log(), "getHttp: " + url);

    HttpHelper helper(url, getTimeout(ctx, url));
    
    appendHeaders(helper, ctx->request(), invoke_ctx);
    httpCall(helper);
    checkStatus(helper);
    
    createTagInfo(helper, invoke_ctx);

    if (invoke_ctx->haveCachedCopy() && !invoke_ctx->tag().modified) {
        return XmlDocHelper();
    }

    return response(helper);
}


XmlDocHelper
HttpBlock::getBinaryPage(Context *ctx, InvokeContext *invoke_ctx) {
    (void)invoke_ctx;

    log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());

    const std::vector<Param*> &p = params();
    unsigned int size = p.size();
    
    if (size == 0 || tagged()) {
        throw InvokeError("bad arity");
    }
    
    std::string url = concatParams(ctx, 0, size - 1);
    PROFILER(log(), "getBinaryPage: " + url);
    
    HttpHelper helper(url, getTimeout(ctx, url));
    
    appendHeaders(helper, ctx->request(), NULL);
    httpCall(helper);

    long status = helper.status();
    if (status != 200) {
        RetryInvokeError error("Incorrect http status", "url", url);
        error.add("status", boost::lexical_cast<std::string>(status));
        throw error;
    }

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
HttpBlock::postHttp(Context *ctx, InvokeContext *invoke_ctx) {

    log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());

    const std::vector<Param*> &p = params();
    unsigned int size = p.size();
    if (size < 2) {
        throwBadArityError();
    }
    
    std::string url = concatParams(ctx, 0, size - 2);
    
    HttpHelper helper(url, getTimeout(ctx, url));
    
    appendHeaders(helper, ctx->request(), invoke_ctx);

    std::string body = p[size-1]->asString(ctx);
    helper.postData(body.data(), body.size());

    httpCall(helper);
    checkStatus(helper);

    createTagInfo(helper, invoke_ctx);

    if (invoke_ctx->haveCachedCopy() && !invoke_ctx->tag().modified) {
        return XmlDocHelper();
    }

    return response(helper);
}

XmlDocHelper
HttpBlock::postByRequest(Context *ctx, InvokeContext *invoke_ctx) {
    (void)invoke_ctx;
    
    log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());

    const std::vector<Param*> &p = params();
    unsigned int size = p.size();
    if (size == 0 || tagged()) {
        throw InvokeError("bad arity");
    }
    
    std::string url = concatParams(ctx, 0, size - 1);
    
    bool is_get = (strcmp(ctx->request()->getRequestMethod().c_str(), "GET") == 0);
    
    if (!is_get) {
        const std::string &query = ctx->request()->getQueryString();
        if (!query.empty()) {
            url.push_back(url.find('?') != std::string::npos ? '&' : '?');
            url.append(query);
        }
    }
    
    HttpHelper helper(url, getTimeout(ctx, url));
    appendHeaders(helper, ctx->request(), NULL);

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
    
    return response(helper);
}

XmlDocHelper
HttpBlock::getByState(Context *ctx, InvokeContext *invoke_ctx) {
    (void)invoke_ctx;

    log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());
    
    const std::vector<Param*> &p = params();
    unsigned int size = p.size();
    
    if (size == 0 || tagged()) {
        throw InvokeError("bad arity");
    }
    
    std::string url = concatParams(ctx, 0, size - 1);
    
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
    
    appendHeaders(helper, ctx->request(), NULL);
    httpCall(helper);
    checkStatus(helper);
    
    return response(helper);
}

XmlDocHelper
HttpBlock::getByRequest(Context *ctx, InvokeContext *invoke_ctx) {
    (void)invoke_ctx;

    log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());

    const std::vector<Param*> &p = params();
    unsigned int size = p.size();
    
    if (size == 0 || tagged()) {
        throw InvokeError("bad arity");
    }
    
    std::string url = concatParams(ctx, 0, size - 1);
    
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
    
    appendHeaders(helper, ctx->request(), NULL);
    httpCall(helper);
    checkStatus(helper);
    
    return response(helper);
}

void
HttpBlock::appendHeaders(HttpHelper &helper, const Request *request, InvokeContext *invoke_ctx) const {
    std::vector<std::string> headers;
    bool real_ip = false;
    const std::string& ip_header_name = Policy::realIPHeaderName();
    if (proxy_ && request->countHeaders() > 0) {    
        std::vector<std::string> names;
        request->headerNames(names);
        for (std::vector<std::string>::const_iterator i = names.begin(), end = names.end(); i != end; ++i) {
            const std::string& name = *i;
            if (name.empty()) {
                continue;
            }
            const std::string& value = request->getHeader(name);
            if (Policy::isSkippedProxyHeader(name)) {
                log()->debug("%s, skipped %s: %s", BOOST_CURRENT_FUNCTION, name.c_str(), value.c_str());
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
    
    if (!real_ip && !ip_header_name.empty()) {
        headers.push_back(ip_header_name);
        headers.back().append(": ").append(request->getRealIP());
    }
    
    helper.appendHeaders(headers,
            invoke_ctx && invoke_ctx->tagged() ? invoke_ctx->tag().last_modified : Tag::UNDEFINED_TIME);
}

XmlDocHelper
HttpBlock::response(const HttpHelper &helper) const {

    boost::shared_ptr<std::string> str = helper.content();
    if (helper.isXml()) {
        return XmlDocHelper(xmlReadMemory(str->c_str(), str->size(), "",
                                          charset_.empty() ? NULL : charset_.c_str(),
                                          XML_PARSE_DTDATTR | XML_PARSE_NOENT));
    }
    if (helper.contentType() == "text/plain") {
        if (str->empty()) {
            XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
            if (NULL != doc.get()) {
                XmlNodeHelper node(xmlNewDocNode(doc.get(), NULL, (const xmlChar*)"text", NULL));
                if (node.get() != NULL) {
                    xmlDocSetRootElement(doc.get(), node.release());
                }
            }
            return doc;
        }
        std::string res;
        res.append("<text>").append(XmlUtils::escape(*str)).append("</text>");
        return XmlDocHelper(xmlReadMemory(res.c_str(), res.size(), "", NULL, XML_PARSE_DTDATTR | XML_PARSE_NOENT));
    }
    if (helper.contentType() == "text/html") {
        std::string data = XmlUtils::sanitize(*str, StringUtils::EMPTY_STRING, 0);
        return XmlDocHelper(xmlReadMemory(data.c_str(), data.size(), helper.base().c_str(),
                                          helper.charset().c_str(),
                                          XML_PARSE_DTDATTR | XML_PARSE_NOENT));
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
HttpBlock::getTimeout(Context *ctx, const std::string &url) {
    int timeout = remainedTime(ctx);
    if (timeout > 0) {
        return timeout;
    }
    
    InvokeError error("block is timed out", "url", url);
    error.add("timeout", boost::lexical_cast<std::string>(ctx->timer().timeout()));
    throw error;
}

void
HttpBlock::checkStatus(const HttpHelper &helper) {
    try {
        helper.checkStatus();
    }
    catch(const std::runtime_error &e) {
        if (print_error_ && helper.content()->size() > 0) {
            XmlDocHelper doc = response(helper);
            if (NULL != doc.get()) {
                xmlNodePtr root_node = xmlDocGetRootElement(doc.get());
                if (root_node) {
                    XmlNodeHelper result_node(xmlCopyNode(root_node, 1));
                    RetryInvokeError error(e.what(), result_node);
                    error.addEscaped("url", helper.url());
                    error.add("status", boost::lexical_cast<std::string>(helper.status()));
                    throw error;
                }
            }
        }
        
        RetryInvokeError error(e.what(), "url", helper.url());
        error.add("status", boost::lexical_cast<std::string>(helper.status()));
        throw error;
    }
}

void
HttpBlock::httpCall(HttpHelper &helper) {
    try {
        helper.perform();
    }
    catch(const std::runtime_error &e) {
        throw RetryInvokeError(e.what(), "url", helper.url());
    }
    log()->debug("%s, http call performed", BOOST_CURRENT_FUNCTION);
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

HttpExtension::HttpExtension() {
}

HttpExtension::~HttpExtension() {
}

const char*
HttpExtension::name() const {
    return "http";
}

const char*
HttpExtension::nsref() const {
    return XmlUtils::XSCRIPT_NAMESPACE;
}

void
HttpExtension::initContext(Context *ctx) {
    (void)ctx;
}

void
HttpExtension::stopContext(Context *ctx) {
    (void)ctx;
}

void
HttpExtension::destroyContext(Context *ctx) {
    (void)ctx;
}

std::auto_ptr<Block>
HttpExtension::createBlock(Xml *owner, xmlNodePtr node) {
    return std::auto_ptr<Block>(new HttpBlock(this, owner, node));
}

void
HttpExtension::init(const Config *config) {
    (void)config;
    try {
        HttpHelper::init();
    }
    catch (const std::exception &e) {
        std::string error_msg("HttpExtension construction: caught exception: ");
        error_msg += e.what();
        terminate(1, error_msg.c_str(), true);
    }
    catch (...) {
        terminate(1, "HttpExtension construction: caught unknown exception", true);
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
static ExtensionRegisterer ext_(ExtensionHolder(new HttpExtension()));

} // namespace xscript


extern "C" ExtensionInfo* get_extension_info() {
    static ExtensionInfo info = { "http", xscript::XmlUtils::XSCRIPT_NAMESPACE };
    return &info;
}

