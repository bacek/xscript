#include "settings.h"
#include "http_block.h"

#include <strings.h>

#include <algorithm>
#include <cassert>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>

#include <boost/tokenizer.hpp>
#include <boost/checked_delete.hpp>

#include <libxml/HTMLparser.h>

#include "http_extension.h"

#include "xscript/args.h"
#include "xscript/context.h"
#include "xscript/encoder.h"
#include "xscript/http_helper.h"
#include "xscript/logger.h"
#include "xscript/meta.h"
#include "xscript/operation_mode.h"
#include "xscript/param.h"
#include "xscript/policy.h"
#include "xscript/profiler.h"
#include "xscript/range.h"
#include "xscript/request.h"
#include "xscript/state.h"
#include "xscript/string_utils.h"
#include "xscript/util.h"
#include "xscript/writer.h"
#include "xscript/xml.h"
#include "xscript/xml_util.h"
#include "xscript/json2xml.h"

#include "internal/parser.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif


namespace xscript {

typedef std::map<std::string, HttpMethod, StringCILess> MethodMap;


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
static const std::string CONTENT_TYPE_HEADER_NAME("Content-Type");
static const std::string KEEP_ALIVE_HEADER_NAME("Connection");
static const std::string KEEP_ALIVE_HEADER_VALUE("Keep-Alive");
static const std::string STR_PARAM_ID("param-id");
static const std::string STR_HEADERS("headers");
static const std::string STR_HEADER_PARAM_NAME("header");
static const std::string STR_HEADER_PARAM_REPR("header param");
static const std::string STR_QUERY_PARAMS("query-params");
static const std::string STR_QUERY_PARAM_NAME("query-param");
static const std::string STR_QUERY_PARAM_REPR("query param");
static const std::string STR_URL("URL");
static const std::string STR_GET = "get";
static const std::string STR_HEAD = "head";
static const std::string STR_DELETE = "delete";
static const std::string STR_PUT = "put";
static const std::string STR_POST = "post";
static const std::string STR_CRLF = "\r\n";
static const std::string STR_LWR_URL = "url";
static const std::string STR_LWR_STATUS = "status";
static const std::string STR_LWR_TIMEOUT = "timeout";
static const std::string STR_LWR_CONTENT_TYPE = "content-type";
static const std::string STR_MULTIPART_BODY_END = "--\r\n";
static const std::string STR_BLOCK_TIMEOUT = "block is timed out";
static const std::string SKIP_CACHE_MESSAGE = "can not cache post data with attached files";
static const std::string TAG_IS_NOT_ALLOWED = "tag is not allowed";

enum { PARSE_FLAGS_NONE = 0, PARSE_FLAGS_XML, PARSE_FLAGS_TEXT };


class HttpHeadersArgList : public CheckedStringArgList {
public:
    explicit HttpHeadersArgList(bool checked) : CheckedStringArgList(checked) {}

    virtual void add(const std::string &value) {
        std::string::size_type pos = value.find_first_of(STR_CRLF);
        if (pos == std::string::npos) {
            CheckedStringArgList::add(value);
        }
        else {
            CheckedStringArgList::add(value.substr(0, pos));
        }
    }
};


HttpBlock::HttpBlock(const HttpExtension *ext, Xml *owner, xmlNodePtr node) :
        Block(ext, owner, node), RemoteTaggedBlock(ext, owner, node),
        method_(NULL), parse_flags_(PARSE_FLAGS_NONE), proxy_(false), xff_(false), print_error_(false),
        load_entities_(ext->loadEntities())
{
}

HttpBlock::~HttpBlock() {
    std::for_each(headers_.begin(), headers_.end(), boost::checked_deleter<Param>());
}

void
HttpBlock::parseSubNode(xmlNodePtr node) {

    if (NULL == node->name || !*node->name) {
        Block::parseSubNode(node);
        return;
    }

    const xmlChar *ref = node->ns ? node->ns->href : NULL;
    if (NULL != ref && 0 != strcasecmp((const char*) ref, XmlUtils::XSCRIPT_NAMESPACE)) {
        Block::parseSubNode(node);
        return;
    }

    if (!strcasecmp((const char*) node->name, STR_HEADER_PARAM_NAME.c_str())) {
        std::auto_ptr<Param> p = createParam(node, "string");
        const std::string &id = p->id();
        log()->debug("creating %s %s in http-block: %s", STR_HEADER_PARAM_REPR.c_str(), id.c_str(), owner()->name().c_str());
        checkHeaderParamId(STR_HEADER_PARAM_REPR, id);
        header_names_.insert(id);
        headers_.push_back(p.get());
        p.release();
        return;
    }

    if (strcasecmp((const char*) node->name, STR_QUERY_PARAM_NAME.c_str())) {
        Block::parseSubNode(node);
        return;
    }

    QueryParamData data(createUncheckedParam(node, "string"));
    const Param *p = data.param();
    const char *t = p->type();
    if (!strcasecmp(t, "requestdata") || !strcasecmp(t, "request")) {
        throw std::invalid_argument(STR_QUERY_PARAM_REPR + " " + t + " disallowed in http block");
    }
    const std::string &id = p->id();
    log()->debug("creating %s %s in http-block: %s", STR_QUERY_PARAM_REPR.c_str(), id.c_str(), owner()->name().c_str());
    data.parse(node);
    checkQueryParamId(STR_QUERY_PARAM_REPR, id);
    query_params_.push_back(data);
}

void
HttpBlock::postParse() {

    if (proxy_ && tagged()) {
        log()->warn("switch off tagging in proxy http-block: %s", owner()->name().c_str());
        tagged(false);
    }

    RemoteTaggedBlock::postParse();

    createCanonicalMethod("http.");

    MethodMap::iterator i = methods_.find(method());
    if (methods_.end() == i) {
        throw std::invalid_argument("nonexistent http method call: " + method());
    }
    method_ = i->second;
    if (method_ != &HttpBlock::get && tagged()) {
        //TODO: throw std::invalid_argument(...) for development
        log()->error("switch off tagging in non-get http-block: %s method: %s", owner()->name().c_str(), i->first.c_str());
        tagged(false);
    }
}

void
HttpBlock::retryCall(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const throw (std::exception) {
    invoke_ctx->resultDoc((this->*method_)(ctx.get(), invoke_ctx.get()));
}

void
HttpBlock::property(const char *name, const char *value) {

    if (!strncasecmp(name, "proxy", sizeof("proxy"))) {
        proxy_ = !strncasecmp(value, "yes", sizeof("yes"));
    }
    else if (!strncasecmp(name, "encoding", sizeof("encoding"))) {
        charset_ = value;
    }
    else if (!strncasecmp(name, "x-forwarded-for", sizeof("x-forwarded-for"))) {
        xff_ = !strncasecmp(value, "yes", sizeof("yes"));
    }
    else if (!strncasecmp(name, "print-error-body", sizeof("print-error-body"))) {
        print_error_ = !strncasecmp(value, "yes", sizeof("yes"));
    }
    else if (!strncasecmp(name, "load-entities", sizeof("load-entities"))) {
        load_entities_ = !strncasecmp(value, "yes", sizeof("yes"));
    }
    else if (!strncasecmp(name, "parse", sizeof("parse"))) {
        if (!strncasecmp(value, "xml", sizeof("xml"))) {
            parse_flags_ = PARSE_FLAGS_XML;
        }
        else if (!strncasecmp(value, "text", sizeof("text"))) {
            parse_flags_ = PARSE_FLAGS_TEXT;
        }
        else {
            RemoteTaggedBlock::property(name, value);
        }
    }
    else {
        RemoteTaggedBlock::property(name, value);
    }
}

std::string
HttpBlock::info(const Context *ctx) const {

    std::string info = RemoteTaggedBlock::info(ctx);
    if (!query_params_.empty()) {
        info.reserve(info.size() + query_params_.size() * 64);
        info.append(" | Query params:");
        for (QueryParams::const_iterator it = query_params_.begin(), end = query_params_.end(); it != end; ++it) {
            info.append(it->info());
        }
    }
    if (!headers_.empty()) {
        info.reserve(info.size() + headers_.size() * 64);
        info.append(" | Headers:");
        for (unsigned int i = 0, n = headers_.size(); i < n; ++i) {
            info.push_back(' ');
            const Param *param = headers_[i];
            info.append(param->id());
            info.push_back(':');
            info.append(param->type());
            const std::string &value = param->value();
            if (!value.empty()) {
                info.push_back('(');
                info.append(value);
                info.push_back(')');
            }
        }
    }
    return info;
}

std::string
HttpBlock::createTagKey(const Context *ctx, const InvokeContext *invoke_ctx) const {

    std::string query_key;
    if (!query_params_.empty()) {
        const QueryParamsArgList *args = dynamic_cast<const QueryParamsArgList*>(invoke_ctx->getExtraArgList(STR_QUERY_PARAMS));
        assert(args);
        if (args->multipart) {
            throw SkipCacheException(SKIP_CACHE_MESSAGE);
        }
        assert(args->size() == query_params_.size());
        unsigned int i = 0;
        std::string query_key_data;
        for (QueryParams::const_iterator it = query_params_.begin(), end = query_params_.end(); it != end; ++it, ++i) {
            std::string value = it->queryStringValue(*args, i);
            if (!value.empty()) {
                if (query_key_data.empty()) {
                    query_key_data.push_back('&');
                }
                query_key_data.append(value);
            }
        }

        if (!query_key_data.empty()) {
            query_key.assign("| Query params:").append(query_key_data);
        }
    }

    std::string key = RemoteTaggedBlock::createTagKey(ctx, invoke_ctx);
    key.append(query_key);

    if (!headers_.empty()) {
        key.append("| Headers:");
        const ArgList *args = invoke_ctx->getExtraArgList(STR_HEADERS);
        assert(args);
        assert(args->size() == headers_.size());
        unsigned int i = 0;
        for (std::vector<Param*>::const_iterator it = headers_.begin(), end = headers_.end(); it != end; ++it, ++i) {
            const Param *p = *it;
            if (i) {
                key.push_back(',');
            }
            key.append(p->id());
            key.push_back('=');
            key.append(args->at(i));
        }
    }
    return key;
}

ArgList*
HttpBlock::createArgList(Context *ctx, InvokeContext *invoke_ctx) const {

    if (!headers_.empty()) {
        boost::shared_ptr<ArgList> args(new HttpHeadersArgList(HttpExtension::checkedHeaders()));
        for (std::vector<Param*>::const_iterator it = headers_.begin(), end = headers_.end(); it != end; ++it) {
            const Param *p = *it;
            try {
                p->add(ctx, *args);
            }
            catch (const CriticalInvokeError &e) {
                throw CriticalInvokeError(STR_HEADER_PARAM_REPR + " error: " + e.what(), STR_PARAM_ID, p->id());
            }
            catch (const std::exception &e) {
                throw InvokeError(STR_HEADER_PARAM_REPR + " error: " + e.what(), STR_PARAM_ID, p->id());
            }
        }
        invoke_ctx->setExtraArgList(STR_HEADERS, args);
    }
    if (!query_params_.empty()) {
        boost::shared_ptr<QueryParamsArgList> args(new QueryParamsArgList(HttpExtension::checkedQueryParams()));
        if (method_ == &HttpBlock::post || method_ == &HttpBlock::put) {
            for (QueryParams::const_iterator it = query_params_.begin(), end = query_params_.end(); it != end; ++it) {
                if (NULL != it->files(ctx)) {
                    args->multipart = true;
                }
            }
        }
        for (QueryParams::const_iterator it = query_params_.begin(), end = query_params_.end(); it != end; ++it) {
            const Param *p = it->param();
            try {
                it->add(ctx, *args);
            }
            catch (const CriticalInvokeError &e) {
                throw CriticalInvokeError(STR_QUERY_PARAM_REPR + " error: " + e.what(), STR_PARAM_ID, p->id());
            }
            catch (const std::exception &e) {
                throw InvokeError(STR_QUERY_PARAM_REPR + " error: " + e.what(), STR_PARAM_ID, p->id());
            }
        }
        invoke_ctx->setExtraArgList(STR_QUERY_PARAMS, args);
    }
    return RemoteTaggedBlock::createArgList(ctx, invoke_ctx);
}

std::string
HttpBlock::getUrl(const ArgList *args, unsigned int first, unsigned int last) const {
    std::string url = Block::concatArguments(args, first, last);
    if (!strncasecmp(url.c_str(), "file://", sizeof("file://") - 1)) {
        throw InvokeError("File scheme is not allowed", STR_LWR_URL, url);
    }
    return url;
}

static std::string
createQueryParamsString(const QueryParams &query_params, const QueryParamsArgList &qargs) {
    unsigned int sz = qargs.size();
    assert(sz == query_params.size());

    std::string val;
    QueryParams::const_iterator it = query_params.begin();
    for (unsigned int i = 0; i != sz; ++i, ++it) {
        const std::string &v = it->queryStringValue(qargs, i);
        if (!v.empty()) {
            val.reserve(val.size() + v.size() + 1);
            if (!val.empty()) {
                val.push_back('&');
            }
            val.append(v);
        }
    }
    return val;
}

std::string
HttpBlock::queryParams(const InvokeContext *invoke_ctx) const {

    const ArgList *args = invoke_ctx->getExtraArgList(STR_QUERY_PARAMS);
    if (NULL == args) {
        return StringUtils::EMPTY_STRING;
    }
    const QueryParamsArgList *qargs = dynamic_cast<const QueryParamsArgList*>(args);
    assert(qargs);
    return createQueryParamsString(query_params_, *qargs);
}

bool
HttpBlock::createPostData(const Context *ctx, const InvokeContext *invoke_ctx, std::string &result) const {

    const ArgList *args = invoke_ctx->getExtraArgList(STR_QUERY_PARAMS);
    if (NULL == args) {
        return false;
    }

    const QueryParamsArgList *qargs = dynamic_cast<const QueryParamsArgList*>(args);
    assert(qargs);
    if (!qargs->multipart) {
        createQueryParamsString(query_params_, *qargs).swap(result);
        return false;
    }
    const std::string &ctype = ctx->request()->getHeader(CONTENT_TYPE_HEADER_NAME);
    std::string boundary = Parser::getBoundary(createRange(ctype));

    unsigned int sz = qargs->size();
    assert(sz == query_params_.size());

    const Request *req = ctx->request();
    assert(req);

    std::vector<std::string> values;
    values.reserve(sz);
    QueryParams::const_iterator it = query_params_.begin();
    for (unsigned int i = 0; i != sz; ++i, ++it) {
        std::string v = it->multipartValue(*qargs, i, boundary, *req);
        if (!v.empty()) {
            values.push_back(StringUtils::EMPTY_STRING);
            values.back().swap(v);
        }
    }

    size_t res_size = boundary.size() + STR_MULTIPART_BODY_END.size();
    for (std::vector<std::string>::const_iterator it = values.begin(); it != values.end(); ++it) {
        res_size += it->size();
    }

    std::string res;
    res.reserve(res_size);
    for (std::vector<std::string>::const_iterator j = values.begin(); j != values.end(); ++j) {
        res.append(*j);
    }
    res.append(boundary).append(STR_MULTIPART_BODY_END);
    res.swap(result);
    return true;
}

static XmlDocHelper
emptyDoc() {
    XmlDocHelper result(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != result.get());
    XmlNodeHelper node(xmlNewDocNode(result.get(), NULL, (const xmlChar*)"empty", NULL));
    XmlUtils::throwUnless(NULL != node.get());
    xmlDocSetRootElement(result.get(), node.release());
    return result;
}

std::string
HttpBlock::createCustomGetUrl(const InvokeContext *invoke_ctx) const {
    const ArgList *args = invoke_ctx->getArgList();
    unsigned int size = args->size();
    if (!size) {
        throwBadArityError();
    }
    
    std::string url = getUrl(args, 0, size - 1);
    std::string query = queryParams(invoke_ctx);
    if (!query.empty()) {
        url.reserve(url.size() + query.size() + 1);
        url.push_back(url.find('?') != std::string::npos ? '&' : '?');
        url.append(query);
    }
    return url;
}

XmlDocHelper
HttpBlock::head(Context *ctx, InvokeContext *invoke_ctx) const {
    log()->info("HttpBlock:head, %s", owner()->name().c_str());

    if (tagged()) {
        throw CriticalInvokeError(TAG_IS_NOT_ALLOWED);
    }

    std::string url = createCustomGetUrl(invoke_ctx);
    PROFILER(log(), "http.head: " + url);

    HttpHelper helper(url, getTimeout(ctx, url));
    appendHeaders(helper, ctx->request(), invoke_ctx, false, false);
    helper.method(STR_HEAD);

    httpCall(helper);
    checkStatus(helper);
    
    createMeta(helper, invoke_ctx);
    return emptyDoc();
}

XmlDocHelper
HttpBlock::get(Context *ctx, InvokeContext *invoke_ctx) const {
    log()->info("HttpBlock:get, %s", owner()->name().c_str());

    std::string url = createCustomGetUrl(invoke_ctx);
    PROFILER(log(), "http.get: " + url);

    HttpHelper helper(url, getTimeout(ctx, url));
    appendHeaders(helper, ctx->request(), invoke_ctx, true, false);

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
HttpBlock::del(Context *ctx, InvokeContext *invoke_ctx) const {
    log()->info("HttpBlock:delete, %s", owner()->name().c_str());

    if (tagged()) {
        throw CriticalInvokeError(TAG_IS_NOT_ALLOWED);
    }

    std::string url = createCustomGetUrl(invoke_ctx);
    PROFILER(log(), "http.delete: " + url);

    HttpHelper helper(url, getTimeout(ctx, url));
    appendHeaders(helper, ctx->request(), invoke_ctx, false, false);
    helper.method(STR_DELETE);

    httpCall(helper);
    checkStatus(helper);

    createMeta(helper, invoke_ctx);
    return response(helper);
}

XmlDocHelper
HttpBlock::customPost(const std::string &method, Context *ctx, InvokeContext *invoke_ctx) const {

    log()->info("HttpBlock:%s, %s", method.c_str(), owner()->name().c_str());

    if (tagged()) {
        throw CriticalInvokeError(TAG_IS_NOT_ALLOWED);
    }

    const ArgList* args = invoke_ctx->getArgList();
    unsigned int size = args->size();
    if (size < 1) {
        throwBadArityError();
    }

    std::string url = getUrl(args, 0, size - 1);
    PROFILER(log(), "http." + method + ": " + url);
    
    HttpHelper helper(url, getTimeout(ctx, url));

    std::string post_data;
    bool multipart = createPostData(ctx, invoke_ctx, post_data);

    appendHeaders(helper, ctx->request(), invoke_ctx, false, multipart);
    helper.postData(post_data.data(), post_data.size());
    helper.method(method);

    httpCall(helper);
    checkStatus(helper);

    createMeta(helper, invoke_ctx);
    return response(helper);
}

XmlDocHelper
HttpBlock::put(Context *ctx, InvokeContext *invoke_ctx) const {
    return customPost(STR_PUT, ctx, invoke_ctx);
}

XmlDocHelper
HttpBlock::post(Context *ctx, InvokeContext *invoke_ctx) const {
    return customPost(STR_POST, ctx, invoke_ctx);
}

XmlDocHelper
HttpBlock::getBinaryPage(Context *ctx, InvokeContext *invoke_ctx) const {
    log()->info("HttpBlock::getBinaryPage, %s", owner()->name().c_str());

    if (tagged()) {
        throw CriticalInvokeError(TAG_IS_NOT_ALLOWED);
    }

    std::string url = createCustomGetUrl(invoke_ctx);
    PROFILER(log(), "http.getBinaryPage: " + url);
    
    HttpHelper helper(url, getTimeout(ctx, url));
    
    appendHeaders(helper, ctx->request(), invoke_ctx, false, false);
    httpCall(helper);

    if (!helper.isOk()) {
        long status = helper.status();
        RetryInvokeError error("Incorrect http status", STR_LWR_URL, url);
        error.add(STR_LWR_STATUS, boost::lexical_cast<std::string>(status));
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
        ctx->response()->setHeader(CONTENT_TYPE_HEADER_NAME, content_type);
    }        
    xmlNewProp(node.get(), (const xmlChar*)STR_LWR_URL.c_str(), (const xmlChar*)XmlUtils::escape(url).c_str());
    xmlDocSetRootElement(doc.get(), node.release());

    return doc;
}

XmlDocHelper
HttpBlock::customPostHttp(const std::string &method, Context *ctx, InvokeContext *invoke_ctx) const {

    log()->info("HttpBlock:%sHttp, %s", method.c_str(), owner()->name().c_str());

    if (tagged()) {
        throw CriticalInvokeError(TAG_IS_NOT_ALLOWED);
    }

    const ArgList* args = invoke_ctx->getArgList();
    unsigned int size = args->size();
    if (size < 1) {
        throwBadArityError();
    }

    std::string url = getUrl(args, 0, size - 2);
    std::string query = queryParams(invoke_ctx);
    if (!query.empty() && size > 1) {
        url.push_back(url.find('?') != std::string::npos ? '&' : '?');
        url.append(query);
    }
    PROFILER(log(), "http." + method + "Http: " + url);
    
    HttpHelper helper(url, getTimeout(ctx, url));
    
    appendHeaders(helper, ctx->request(), invoke_ctx, false, false);

    if (size == 1) {
        helper.postData(query.data(), query.size());
    }
    else {
        const std::string& body = args->at(size-1);
        helper.postData(body.data(), body.size());
    }
    helper.method(method);

    httpCall(helper);
    checkStatus(helper);

    createMeta(helper, invoke_ctx);
    return response(helper);
}

XmlDocHelper
HttpBlock::putHttp(Context *ctx, InvokeContext *invoke_ctx) const {
    return customPostHttp(STR_PUT, ctx, invoke_ctx);
}

XmlDocHelper
HttpBlock::postHttp(Context *ctx, InvokeContext *invoke_ctx) const {
    return customPostHttp(STR_POST, ctx, invoke_ctx);
}


XmlDocHelper
HttpBlock::customPostByRequest(const std::string &method, Context *ctx, InvokeContext *invoke_ctx) const {
    
    log()->info("HttpBlock::%sByRequest, %s", method.c_str(), owner()->name().c_str());

    if (tagged()) {
        throw CriticalInvokeError(TAG_IS_NOT_ALLOWED);
    }

    const ArgList* args = invoke_ctx->getArgList();
    unsigned int size = args->size();
    if (!size) {
        throwBadArityError();
    }

    std::string url = getUrl(args, 0, size - 1);
    bool has_query = url.find('?') != std::string::npos;

    bool is_postdata = ctx->request()->hasPostData();
    if (is_postdata) {
        const std::string &query = ctx->request()->getQueryString();
        if (!query.empty()) {
            url.push_back(has_query ? '&' : '?');
            url.append(query);
            has_query = true;
        }
    }
    std::string query = queryParams(invoke_ctx);
    if (!query.empty()) {
        url.push_back(has_query ? '&' : '?');
        url.append(query);
    }
    
    PROFILER(log(), "http." + method + "ByRequest: " + url);
    
    HttpHelper helper(url, getTimeout(ctx, url));
    appendHeaders(helper, ctx->request(), invoke_ctx, false, is_postdata);

    if (is_postdata) {
        std::pair<const char*, std::streamsize> body = ctx->request()->requestBody();
        helper.postData(body.first, body.second);
    }
    else {
        const std::string &query = ctx->request()->getQueryString();
        helper.postData(query.c_str(), query.length());
    }
    helper.method(method);

    httpCall(helper);
    checkStatus(helper);
    createMeta(helper, invoke_ctx);

    return response(helper);
}

XmlDocHelper
HttpBlock::putByRequest(Context *ctx, InvokeContext *invoke_ctx) const {
    return customPostByRequest(STR_PUT, ctx, invoke_ctx);
}

XmlDocHelper
HttpBlock::postByRequest(Context *ctx, InvokeContext *invoke_ctx) const {
    return customPostByRequest(STR_POST, ctx, invoke_ctx);
}

XmlDocHelper
HttpBlock::getByState(Context *ctx, InvokeContext *invoke_ctx) const {

    log()->info("HttpBlock::getByState, %s", owner()->name().c_str());
    
    if (tagged()) {
        throw CriticalInvokeError(TAG_IS_NOT_ALLOWED);
    }

    const ArgList* args = invoke_ctx->getArgList();
    unsigned int size = args->size();
    if (!size) {
        throwBadArityError();
    }

    std::string url = getUrl(args, 0, size - 1);
    bool has_query = url.find('?') != std::string::npos;

    std::string query = queryParams(invoke_ctx);
    if (!query.empty()) {
        url.push_back(has_query ? '&' : '?');
        url.append(query);
        has_query = true;
    }

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
    PROFILER(log(), "http.getByState: " + url);
    
    HttpHelper helper(url, getTimeout(ctx, url));
    
    appendHeaders(helper, ctx->request(), invoke_ctx, false, false);
    httpCall(helper);
    checkStatus(helper);
    createMeta(helper, invoke_ctx);
    
    return response(helper);
}

std::string
HttpBlock::createGetByRequestUrl(const Context *ctx, const InvokeContext *invoke_ctx) const {

    if (tagged()) {
        throw CriticalInvokeError(TAG_IS_NOT_ALLOWED);
    }

    const ArgList* args = invoke_ctx->getArgList();
    unsigned int size = args->size();
    if (!size) {
        throwBadArityError();
    }
    std::string url = getUrl(args, 0, size - 1);
    bool has_query = url.find('?') != std::string::npos;

    if (ctx->request()->hasPostData()) {
        const std::vector<StringUtils::NamedValue>& args = ctx->request()->args();
        if (!args.empty()) {
            for(std::vector<StringUtils::NamedValue>::const_iterator it = args.begin(), end = args.end();
                it != end;
                ++it) {
                url.push_back(has_query ? '&' : '?');
                url.append(it->first);
                url.push_back('=');
                url.append(StringUtils::urlencode(it->second)); // was: without urlencode()
                has_query = true;
            }
        }
    }
    else {
        const std::string &query = ctx->request()->getQueryString();
        if (!query.empty()) {
            url.push_back(has_query ? '&' : '?');
            url.append(query);
            has_query = true;
        }
    }
    std::string query = queryParams(invoke_ctx);
    if (!query.empty()) {
        url.push_back(has_query ? '&' : '?');
        url.append(query);
    }
    return url;
}

XmlDocHelper
HttpBlock::headByRequest(Context *ctx, InvokeContext *invoke_ctx) const {

    log()->info("HttpBlock:headByRequest, %s", owner()->name().c_str());

    std::string url = createGetByRequestUrl(ctx, invoke_ctx);
    PROFILER(log(), "http.headByRequest: " + url);
    
    HttpHelper helper(url, getTimeout(ctx, url));
    
    appendHeaders(helper, ctx->request(), invoke_ctx, false, false);
    helper.method(STR_HEAD);

    httpCall(helper);
    checkStatus(helper);

    createMeta(helper, invoke_ctx);
    return emptyDoc();
}

XmlDocHelper
HttpBlock::customGetByRequest(const std::string &method, Context *ctx, InvokeContext *invoke_ctx) const {

    log()->info("HttpBlock:%sByRequest, %s", method.c_str(), owner()->name().c_str());

    std::string url = createGetByRequestUrl(ctx, invoke_ctx);
    PROFILER(log(), "http." + method + "ByRequest: " + url);
    
    HttpHelper helper(url, getTimeout(ctx, url));
    
    appendHeaders(helper, ctx->request(), invoke_ctx, false, false);
    helper.method(method);

    httpCall(helper);
    checkStatus(helper);

    createMeta(helper, invoke_ctx);
    return response(helper);
}

XmlDocHelper
HttpBlock::getByRequest(Context *ctx, InvokeContext *invoke_ctx) const {
    return customGetByRequest(STR_GET, ctx, invoke_ctx);
}

XmlDocHelper
HttpBlock::deleteByRequest(Context *ctx, InvokeContext *invoke_ctx) const {
    return customGetByRequest(STR_DELETE, ctx, invoke_ctx);
}

inline static void
setHeaderValue(std::string &s, const std::string &name, const std::string &value) {

    s.reserve(name.size() + value.size() + 2);
    s.append(name).append(": ", 2).append(value);
}

inline static void
pushHeaderValue(std::vector<std::string> &headers, const std::string &name, const std::string &value) {

    headers.push_back(StringUtils::EMPTY_STRING);
    setHeaderValue(headers.back(), name, value);
}

void
HttpBlock::appendHeaders(HttpHelper &helper, const Request *request,
        const InvokeContext *invoke_ctx, bool allow_tag, bool pass_ctype) const {

    std::vector<std::string> headers;
    const std::string &xff_header_name = Request::X_FORWARDED_FOR_HEADER_NAME;
    const std::string &ip_header_name = Policy::instance()->realIPHeaderName();
    int ctype_pos = -1;
    bool real_ip = false;
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
            else if (!strcasecmp(xff_header_name.c_str(), name.c_str())) {
                log()->debug("proxy XFF header was skipped (will be recalculated) %s: %s", name.c_str(), value.c_str());
            }
            else {
                if (!real_ip && !strcasecmp(ip_header_name.c_str(), name.c_str())) {
                    real_ip = true;
                }
                else if (pass_ctype && !strcasecmp(name.c_str(), CONTENT_TYPE_HEADER_NAME.c_str())) {
                    ctype_pos = headers.size();
                }
                pushHeaderValue(headers, name, value);
            }
        }
    }

    bool xff = false;
    if (invoke_ctx && !headers_.empty()) {
        const ArgList *args = invoke_ctx->getExtraArgList(STR_HEADERS);
        assert(NULL != args);
        unsigned int sz = args->size();
        assert(sz == headers_.size());
        for (unsigned int i = 0; i != sz; ++i) {
            const Param *param = headers_[i];
            const std::string &name = param->id();
            if (!real_ip && !strcasecmp(ip_header_name.c_str(), name.c_str())) {
                real_ip = true;
            }
            else if (!xff && !strcasecmp(xff_header_name.c_str(), name.c_str())) {
                xff = true;
            }
            else if (pass_ctype && !strcasecmp(name.c_str(), CONTENT_TYPE_HEADER_NAME.c_str())) {
                if (ctype_pos != -1) {
                    setHeaderValue(headers[ctype_pos], name, args->at(i));
                    continue;
                }
                ctype_pos = headers.size();
            }
            pushHeaderValue(headers, name, args->at(i));
        }
    }

    if (pass_ctype && ctype_pos == -1 && request->hasHeader(CONTENT_TYPE_HEADER_NAME)) {
        pushHeaderValue(headers, CONTENT_TYPE_HEADER_NAME, request->getHeader(CONTENT_TYPE_HEADER_NAME));
    }

    if (!real_ip && !ip_header_name.empty()) {
        pushHeaderValue(headers, ip_header_name, request->getRealIP());
    }
    if (!xff && xff_ && !xff_header_name.empty()) {
        pushHeaderValue(headers, xff_header_name, request->getXForwardedFor());
    }
    if (HttpExtension::keepAlive()) {
        pushHeaderValue(headers, KEEP_ALIVE_HEADER_NAME, KEEP_ALIVE_HEADER_VALUE);
    }
    if (allow_tag && invoke_ctx && invoke_ctx->tagged()) {
        helper.appendHeaders(headers, invoke_ctx->tag().last_modified);
    }
    else {
        helper.appendHeaders(headers, Tag::UNDEFINED_TIME);
    }
}

static XmlDocHelper
parseXmlData(const char *buffer, int size, const char *URL, const char *encoding, bool load_entities) {

    if (encoding && !*encoding) {
        encoding = NULL;
    }

    if (load_entities) {
        return XmlDocHelper(xmlReadMemory(buffer, size, URL, encoding, XML_PARSE_DTDATTR | XML_PARSE_NOENT));
    }

    XmlEntityBlocker blocker;
    return XmlDocHelper(xmlReadMemory(buffer, size, URL, encoding, XML_PARSE_DTDATTR | XML_PARSE_NOENT));
}

static XmlDocHelper
responseText(const std::string &str) {

    XmlDocHelper result(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != result.get());
    XmlNodeHelper node(xmlNewDocNode(result.get(), NULL, (const xmlChar*)"text", NULL));
    XmlUtils::throwUnless(NULL != node.get());

    if (!str.empty()) {
        std::string res;
        res.reserve(str.size() * 2 + 20);
        XmlUtils::escape(str, res);
	xmlNodeSetContent(node.get(), (const xmlChar*) res.c_str());
    }
    xmlDocSetRootElement(result.get(), node.release());
    return result;
}

XmlDocHelper
HttpBlock::response(const HttpHelper &helper, bool error_mode) const {

    boost::shared_ptr<std::string> str = helper.content();

    if (!error_mode && (parse_flags_ == PARSE_FLAGS_TEXT)) {
        return responseText(*str);
    }

    if ((!error_mode && (parse_flags_ == PARSE_FLAGS_XML)) || helper.isXml()) {
        XmlDocHelper result(parseXmlData(str->c_str(), str->size(), "", charset_.c_str(), load_entities_));
        XmlUtils::throwUnless(NULL != result.get(), "Url", helper.url().c_str());
        OperationMode::instance()->processXmlError(helper.url());
        return result;
    }

    if (helper.isJson()) {
	XmlDocHelper result = Json2Xml::instance()->createDoc(*str, charset_.empty() ? NULL : charset_.c_str());
	if (NULL != result.get()) {
            return result;
	}
    }
    
    if (!error_mode && helper.isHtml()) {
        std::string data = XmlUtils::sanitize(*str, StringUtils::EMPTY_STRING, 0);
        if (data.empty()) {
            throw InvokeError("Empty sanitized text/html document");
        }

        XmlDocHelper result(parseXmlData(
            data.c_str(), data.size(), helper.base().c_str(), helper.charset().c_str(), load_entities_));

        if (NULL == result.get()) {
            log()->error("Invalid sanitized text/html document. Url: %s", helper.url().c_str());
            std::string error = "Invalid sanitized text/html document. Url: " + helper.url() + " Error: ";
            std::string xml_error = XmlUtils::getXMLError();
            xml_error.empty() ? error.append("Unknown XML error") : error.append(xml_error);
            OperationMode::instance()->processCriticalInvokeError(error);
        }
        
        OperationMode::instance()->processXmlError(helper.url());
        return result;
    }
    
    if (helper.isText()) {
        return responseText(*str);
    }

    if (!error_mode) {
        throw InvokeError("format is not recognized: " + helper.contentType(), STR_LWR_URL, helper.url());
    }
    return XmlDocHelper();
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
    
    InvokeError error(STR_BLOCK_TIMEOUT);
    error.add(STR_LWR_URL, url);
    error.add(STR_LWR_TIMEOUT, boost::lexical_cast<std::string>(ctx->timer().timeout()));
    throw error;
}

void
HttpBlock::wrapError(InvokeError &error, const HttpHelper &helper, const XmlNodeHelper &error_body_node) const {
    error.add(STR_LWR_URL, helper.url());
    error.add(STR_LWR_STATUS, boost::lexical_cast<std::string>(helper.status()));
    if (!helper.contentType().empty()) {
        error.add(STR_LWR_CONTENT_TYPE, helper.contentType());
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
        if (print_error_ && helper.hasContent() && !helper.isHtml()) {
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
        throw RetryInvokeError(e.what(), STR_LWR_URL, helper.url());
    }
    log()->debug("HttpBlock::httpCall, http call performed");
}

void
HttpBlock::createMeta(HttpHelper &helper, InvokeContext *invoke_ctx) const {
    if (!metaBlock()) {
        return;
    }

    typedef std::multimap<std::string, std::string> HttpHeaderMap;
    typedef HttpHeaderMap::const_iterator HttpHeaderIter;

    boost::shared_ptr<Meta> meta = invoke_ctx->meta();
    const HttpHeaderMap& headers = helper.headers();
    for (HttpHeaderIter it = headers.begin(); it != headers.end(); ) {
        std::pair<HttpHeaderIter, HttpHeaderIter> res = headers.equal_range(it->first);
        std::vector<std::string> result;
        HttpHeaderIter itr = res.first;
        std::string name = "HTTP_" + StringUtils::toupper(it->first);
        for (; itr != res.second; ++itr) {
            result.push_back(itr->second);
        }
        result.size() == 1 ?
            meta->setString(name, result[0]) :
            meta->setArray(name, result);
        it = itr;
    }

    meta->setString(STR_URL, helper.url());
    meta->setLong(STR_LWR_STATUS, helper.status());
}

void
HttpBlock::checkHeaderParamId(const std::string &repr_name, const std::string &id) {
    if (id.empty()) {
        throw std::runtime_error(repr_name + " without id");
    }

    int size = id.size();
    if (size > 128) {
        throw UnboundRuntimeError(repr_name + " with too big size id: " + id);
    }

    if (!::isalpha(id[0])) {
        throw std::runtime_error(repr_name + " with incorrect first character in id: " + id);
    }

    for (int i = 1; i < size; ++i) {
        char character = id[i];
        if (character == '-' || ::isalnum(character)) {
            continue;
        }

        std::stringstream ss;
        ss << repr_name << " with incorrect character at " << i + 1 << " in id: " << id;
        throw std::runtime_error(ss.str());
    }
}

void
HttpBlock::checkQueryParamId(const std::string &repr_name, const std::string &id) {
    if (id.empty()) {
        throw std::runtime_error(repr_name + " without id");
    }

    int size = id.size();
    if (size > 128) {
        throw UnboundRuntimeError(repr_name + " with too big size id: " + id);
    }

    for (int i = 0; i < size; ++i) {
        char character = id[i];
        if (character == '-' || character == '_' || ::isalnum(character)) {
            continue;
        }

        std::stringstream ss;
        ss << repr_name << " with incorrect character at " << i + 1 << " in id: " << id;
        throw std::runtime_error(ss.str());
    }
}

void
HttpBlock::registerMethod(const std::string &name, HttpMethod method) {
    try {
        MethodMap::iterator i = methods_.find(name);
        if (methods_.end() != i) {
            throw std::invalid_argument("registering duplicate http method: " + name);
        }

        std::pair<std::string, HttpMethod> p(name, method);
        methods_.insert(p);
    }
    catch (const std::exception &e) {
        xscript::log()->error("HttpBlock::registerMethod, caught exception: %s", e.what());
        throw;
    }
}

HttpMethodRegistrator::HttpMethodRegistrator() {
    HttpBlock::registerMethod("getHttp", &HttpBlock::get);
    HttpBlock::registerMethod("get_http", &HttpBlock::get);
    HttpBlock::registerMethod("getPageT", &HttpBlock::get);
    HttpBlock::registerMethod("curlGetHttp", &HttpBlock::get);

    HttpBlock::registerMethod(STR_GET, &HttpBlock::get);
    HttpBlock::registerMethod(STR_HEAD, &HttpBlock::head);
    HttpBlock::registerMethod(STR_DELETE, &HttpBlock::del);
    HttpBlock::registerMethod(STR_PUT, &HttpBlock::put);
    HttpBlock::registerMethod(STR_POST, &HttpBlock::post);

    HttpBlock::registerMethod("putHttp", &HttpBlock::putHttp);
    HttpBlock::registerMethod("put_http", &HttpBlock::putHttp);

    HttpBlock::registerMethod("postHttp", &HttpBlock::postHttp);
    HttpBlock::registerMethod("post_http", &HttpBlock::postHttp);

    HttpBlock::registerMethod("getByState", &HttpBlock::getByState);
    HttpBlock::registerMethod("get_by_state", &HttpBlock::getByState);

    HttpBlock::registerMethod("headByRequest", &HttpBlock::headByRequest);
    HttpBlock::registerMethod("head_by_request", &HttpBlock::headByRequest);

    HttpBlock::registerMethod("getByRequest", &HttpBlock::getByRequest);
    HttpBlock::registerMethod("get_by_request", &HttpBlock::getByRequest);

    HttpBlock::registerMethod("deleteByRequest", &HttpBlock::deleteByRequest);
    HttpBlock::registerMethod("delete_by_request", &HttpBlock::deleteByRequest);

    HttpBlock::registerMethod("putByRequest", &HttpBlock::putByRequest);
    HttpBlock::registerMethod("put_by_request", &HttpBlock::putByRequest);

    HttpBlock::registerMethod("postByRequest", &HttpBlock::postByRequest);
    HttpBlock::registerMethod("post_by_request", &HttpBlock::postByRequest);

    HttpBlock::registerMethod("getBinaryPage", &HttpBlock::getBinaryPage);
    HttpBlock::registerMethod("get_binary_page", &HttpBlock::getBinaryPage);
}

static HttpMethodRegistrator reg_;

} // namespace xscript
