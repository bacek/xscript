#include "settings.h"
#include "http_block.h"

#include <strings.h>

#include <algorithm>
#include <memory>
#include <cassert>
#include <sstream>
#include <stdexcept>

#include <boost/tokenizer.hpp>
#include <boost/current_function.hpp>
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

#include "internal/hash.h"
#include "internal/hashmap.h"
#include "internal/parser.h"

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

class HttpHeadersArgList : public CheckedStringArgList {
public:
    explicit HttpHeadersArgList(bool checked) : CheckedStringArgList(checked) {}

    virtual void add(const std::string &value) {
        std::string::size_type pos = value.find_first_of("\r\n");
        if (pos == std::string::npos) {
            CheckedStringArgList::add(value);
        }
        else {
            CheckedStringArgList::add(value.substr(0, pos));
        }
    }
};


static MethodMap methods_;
static const std::string CONTENT_TYPE_HEADER_NAME("Content-Type");
static const std::string STR_PARAM_ID("param-id");
static const std::string STR_HEADERS("headers");
static const std::string STR_HEADER_PARAM_NAME("header");
static const std::string STR_HEADER_PARAM_REPR("header param");
static const std::string STR_QUERY_PARAMS("query-params");
static const std::string STR_QUERY_PARAM_NAME("query-param");
static const std::string STR_QUERY_PARAM_REPR("query param");
static const std::string STR_URL("URL");


HttpBlock::HttpBlock(const Extension *ext, Xml *owner, xmlNodePtr node) :
        Block(ext, owner, node), RemoteTaggedBlock(ext, owner, node),
        method_(NULL), proxy_(false), xff_(false), print_error_(false) {
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
    else if (strncasecmp(name, "x-forwarded-for", sizeof("x-forwarded-for")) == 0) {
        xff_ = (strncasecmp(value, "yes", sizeof("yes")) == 0);
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
static const std::string SKIP_CACHE_MESSAGE = "can not cache post data with attached files";

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
        if (method_ == &HttpBlock::post) {
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
    if (strncasecmp(url.c_str(), "file://", sizeof("file://") - 1) == 0) {
        throw InvokeError("File scheme is not allowed", "url", url);
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

    size_t res_size = boundary.size() + 4;
    for (std::vector<std::string>::const_iterator it = values.begin(); it != values.end(); ++it) {
        res_size += it->size();
    }

    std::string res;
    res.reserve(res_size);
    for (std::vector<std::string>::const_iterator j = values.begin(); j != values.end(); ++j) {
        res.append(*j);
    }
    res.append(boundary).append("--\r\n", 4);
    res.swap(result);
    return true;
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
    std::string query = queryParams(invoke_ctx);
    if (!query.empty()) {
        url.push_back(url.find('?') != std::string::npos ? '&' : '?');
        url.append(query);
    }

    PROFILER(log(), "getHttp: " + url);

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
    std::string query = queryParams(invoke_ctx);
    if (!query.empty()) {
        url.push_back(url.find('?') != std::string::npos ? '&' : '?');
        url.append(query);
    }
    PROFILER(log(), "getBinaryPage: " + url);
    
    HttpHelper helper(url, getTimeout(ctx, url));
    
    appendHeaders(helper, ctx->request(), invoke_ctx, false, false);
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
HttpBlock::post(Context *ctx, InvokeContext *invoke_ctx) const {

    log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());

    const ArgList* args = invoke_ctx->getArgList();
    unsigned int size = args->size();
    if (size < 1) {
        throwBadArityError();
    }

    std::string url = getUrl(args, 0, size - 1);
    PROFILER(log(), "post: " + url);
    
    HttpHelper helper(url, getTimeout(ctx, url));
    

    std::string post_data;
    bool multipart = createPostData(ctx, invoke_ctx, post_data);

    appendHeaders(helper, ctx->request(), invoke_ctx, !multipart, multipart);
    helper.postData(post_data.data(), post_data.size());

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
HttpBlock::postHttp(Context *ctx, InvokeContext *invoke_ctx) const {

    log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());

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
    PROFILER(log(), "postHttp: " + url);
    
    HttpHelper helper(url, getTimeout(ctx, url));
    
    appendHeaders(helper, ctx->request(), invoke_ctx, true, false);

    if (size == 1) {
        helper.postData(query.data(), query.size());
    }
    else {
        const std::string& body = args->at(size-1);
        helper.postData(body.data(), body.size());
    }

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
    bool has_query = url.find('?') != std::string::npos;

    const std::string &method = ctx->request()->getRequestMethod();
    bool is_post = !strcasecmp(method.c_str(), "POST") || !strcasecmp(method.c_str(), "PUT");
    if (is_post) {
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
    
    PROFILER(log(), "postByRequest: " + url);
    
    HttpHelper helper(url, getTimeout(ctx, url));
    appendHeaders(helper, ctx->request(), invoke_ctx, false, is_post);

    if (is_post) {
        std::pair<const char*, std::streamsize> body = ctx->request()->requestBody();
        helper.postData(body.first, body.second);
    }
    else {
        const std::string &query = ctx->request()->getQueryString();
        helper.postData(query.c_str(), query.length());
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
    PROFILER(log(), "getByState: " + url);
    
    HttpHelper helper(url, getTimeout(ctx, url));
    
    appendHeaders(helper, ctx->request(), invoke_ctx, false, false);
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
    bool has_query = url.find('?') != std::string::npos;

    const std::string &method = ctx->request()->getRequestMethod();
    if (!strcasecmp(method.c_str(), "POST") || !strcasecmp(method.c_str(), "PUT")) {
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
    PROFILER(log(), "getByRequest: " + url);
    
    HttpHelper helper(url, getTimeout(ctx, url));
    
    appendHeaders(helper, ctx->request(), invoke_ctx, false, false);
    httpCall(helper);
    checkStatus(helper);
    createMeta(helper, invoke_ctx);
    
    return response(helper);
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
    if (allow_tag && invoke_ctx && invoke_ctx->tagged()) {
        helper.appendHeaders(headers, invoke_ctx->tag().last_modified);
    }
    else {
        helper.appendHeaders(headers, Tag::UNDEFINED_TIME);
    }
}

XmlDocHelper
HttpBlock::response(const HttpHelper &helper, bool error_mode) const {

    boost::shared_ptr<std::string> str = helper.content();
    if (helper.isXml()) {
        XmlDocHelper result(xmlReadMemory(str->c_str(), str->size(), "",
            charset_.empty() ? NULL : charset_.c_str(), XML_PARSE_DTDATTR | XML_PARSE_NOENT));
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
    
    if (helper.isText()) {
        if (str->empty()) {
            XmlDocHelper result(xmlNewDoc((const xmlChar*) "1.0"));
            XmlUtils::throwUnless(NULL != result.get());
            XmlNodeHelper node(xmlNewDocNode(result.get(), NULL, (const xmlChar*)"text", NULL));
            XmlUtils::throwUnless(NULL != node.get());
            xmlDocSetRootElement(result.get(), node.release());
            return result;
        }

        std::string res;
        res.reserve(str->size() * 2 + 20);
        res.append("<text>");
        XmlUtils::escape(*str, res);
        res.append("</text>");
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
        throw RetryInvokeError(e.what(), "url", helper.url());
    }
    log()->debug("%s, http call performed", BOOST_CURRENT_FUNCTION);
}

void
HttpBlock::createMeta(HttpHelper &helper, InvokeContext *invoke_ctx) const {
    if (metaBlock()) {
        typedef std::multimap<std::string, std::string> HttpHeaderMap;
        typedef HttpHeaderMap::const_iterator HttpHeaderIter;

        boost::shared_ptr<Meta> meta = invoke_ctx->meta();
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
                meta->setString(name, result[0]) :
                meta->setArray(name, result);
            it = itr;
        }
        meta->setString(STR_URL, helper.url());
    }
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

    HttpBlock::registerMethod("get", &HttpBlock::getHttp);
    HttpBlock::registerMethod("post", &HttpBlock::post);

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
