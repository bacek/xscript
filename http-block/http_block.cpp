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
#include "http_helper.h"

#include "xscript/context.h"
#include "xscript/logger.h"
#include "xscript/param.h"
#include "xscript/profiler.h"
#include "xscript/request.h"
#include "xscript/state.h"
#include "xscript/util.h"
#include "xscript/string_utils.h"
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
        Block(ext, owner, node), RemoteTaggedBlock(ext, owner, node), proxy_(false), method_(NULL) {
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
HttpBlock::call(Context *ctx, boost::any &a) throw (std::exception) {
    return (this->*method_)(ctx, a);
}

void
HttpBlock::property(const char *name, const char *value) {

    if (strncasecmp(name, "proxy", sizeof("proxy")) == 0) {
        proxy_ = (strncasecmp(value, "yes", sizeof("yes")) == 0);
    }
    else if (strncasecmp(name, "encoding", sizeof("encoding")) == 0) {
        charset_ = value;
    }
    else {
        RemoteTaggedBlock::property(name, value);
    }
}

XmlDocHelper
HttpBlock::getHttp(Context *ctx, boost::any &a) {

    log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());

    const std::vector<Param*> &p = params();
    unsigned int size = p.size();
    if (size == 0) {
        throwBadArityError();
    }

    std::string url = concatParams(ctx, 0, size - 1);
    PROFILER(log(), "getHttp: " + url);

    if (strncasecmp(url.c_str(), "file://", sizeof("file://") - 1) == 0) {
        namespace fs = boost::filesystem;
        url.erase(0, sizeof("file://") - 1);
        fs::path file_path(url);
        std::string native_path = file_path.native_file_string();
        
        struct stat st;
        int res = stat(native_path.c_str(), &st);
        if (res != 0) {
            std::stringstream stream;
            StringUtils::report("failed to stat file: ", errno, stream);
            throw InvokeError(stream.str(), "url", url);
        }
        
        bool modified = true;
        if (tagged()) {
            const Tag* tag = boost::any_cast<Tag>(&a);
            if (tag && tag->last_modified != Tag::UNDEFINED_TIME && st.st_mtime == tag->last_modified) {
                modified = false;
            }

            Tag local_tag(modified, st.st_mtime, Tag::UNDEFINED_TIME);
            a = boost::any(local_tag);
        }
        
        XmlDocHelper doc;
        if (modified) {
            doc = XmlDocHelper(xmlParseFile(native_path.c_str()));
            if (doc.get() == NULL) {
                throw InvokeError("got empty document", "url", url);
            }
        }
        return doc;
    }

    const TimeoutCounter &timer = ctx->blockTimer(this);
    checkTimeout(timer, url);

    const Tag *tag = boost::any_cast<Tag>(&a);
    HttpHelper helper(url, timer.remained());
    helper.appendHeaders(ctx->request(), proxy_, tag);

    helper.perform();
    log()->debug("%s, http call performed", BOOST_CURRENT_FUNCTION);
    helper.checkStatus();

    createTagInfo(helper, a);
    const Tag *result_tag = boost::any_cast<Tag>(&a);

    if (result_tag && !result_tag->modified) {
        return XmlDocHelper();
    }

    return response(helper);
}


XmlDocHelper
HttpBlock::getBinaryPage(Context *ctx, boost::any &a) {
    (void)a;

    log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());

    const std::vector<Param*> &p = params();
    unsigned int size = p.size();
    
    if (size == 0 || tagged()) {
        throw InvokeError("bad arity");
    }
    
    std::string url = concatParams(ctx, 0, size - 1);
    PROFILER(log(), "getBinaryPage: " + url);

    const TimeoutCounter &timer = ctx->blockTimer(this);
    checkTimeout(timer, url);

    HttpHelper helper(url, timer.remained());
    helper.appendHeaders(ctx->request(), proxy_, NULL);

    helper.perform();
    log()->debug("%s, http call performed", BOOST_CURRENT_FUNCTION);

    long status = helper.status();
    if (status != 200) {
        InvokeError error("Incorrect http status");
        error.add("status", boost::lexical_cast<std::string>(status));
        error.addEscaped("url", url);
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
HttpBlock::postHttp(Context *ctx, boost::any &a) {

    log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());

    const std::vector<Param*> &p = params();
    unsigned int size = p.size();
    if (size < 2) {
        throwBadArityError();
    }
    
    std::string url = concatParams(ctx, 0, size - 2);

    const TimeoutCounter &timer = ctx->blockTimer(this);
    checkTimeout(timer, url);

    const Tag *tag = boost::any_cast<Tag>(&a);
    HttpHelper helper(url, timer.remained());
    std::string body = p[size-1]->asString(ctx);
    helper.appendHeaders(ctx->request(), proxy_, tag);

    helper.postData(body.data(), body.size());

    helper.perform();
    log()->debug("%s, http call performed", BOOST_CURRENT_FUNCTION);
    helper.checkStatus();

    createTagInfo(helper, a);
    const Tag *result_tag = boost::any_cast<Tag>(&a);

    if (result_tag && !result_tag->modified) {
        return XmlDocHelper();
    }

    return response(helper);
}

XmlDocHelper
HttpBlock::postByRequest(Context *ctx, boost::any &a) {
    (void)a;
    
    log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());

    const std::vector<Param*> &p = params();
    unsigned int size = p.size();
    if (size == 0 || tagged()) {
        throw InvokeError("bad arity");
    }
    
    std::string url = concatParams(ctx, 0, size - 1);
    
    const std::string &query = ctx->request()->getQueryString();
    if (!query.empty()) {
        url.append(1, url.find('?') != std::string::npos ? '&' : '?');
        url.append(query);
    }

    const TimeoutCounter &timer = ctx->blockTimer(this);
    checkTimeout(timer, url);

    HttpHelper helper(url, timer.remained());    
    helper.appendHeaders(ctx->request(), proxy_, NULL);

    std::pair<const char*, std::streamsize> body = ctx->request()->requestBody();
    helper.postData(body.first, body.second);

    helper.perform();
    log()->debug("%s, http call performed", BOOST_CURRENT_FUNCTION);
    helper.checkStatus();
    
    return response(helper);
}

XmlDocHelper
HttpBlock::getByState(Context *ctx, boost::any &a) {
    (void)a;

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

    const TimeoutCounter &timer = ctx->blockTimer(this);
    checkTimeout(timer, url);

    HttpHelper helper(url, timer.remained());
    helper.appendHeaders(ctx->request(), proxy_, NULL);

    helper.perform();
    log()->debug("%s, http call performed", BOOST_CURRENT_FUNCTION);
    helper.checkStatus();

    return response(helper);
}

XmlDocHelper
HttpBlock::getByRequest(Context *ctx, boost::any &a) {
    (void)a;

    log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());

    const std::vector<Param*> &p = params();
    unsigned int size = p.size();
    
    if (size == 0 || tagged()) {
        throw InvokeError("bad arity");
    }
    
    std::string url = concatParams(ctx, 0, size - 1);
    
    const std::string &query = ctx->request()->getQueryString();
    if (!query.empty()) {
        url.append(1, url.find('?') != std::string::npos ? '&' : '?');
        url.append(query);
    }

    const TimeoutCounter &timer = ctx->blockTimer(this);
    checkTimeout(timer, url);

    HttpHelper helper(url, timer.remained());
    helper.appendHeaders(ctx->request(), proxy_, NULL);

    helper.perform();
    log()->debug("%s, http call performed", BOOST_CURRENT_FUNCTION);
    helper.checkStatus();

    return response(helper);
}

XmlDocHelper
HttpBlock::response(const HttpHelper &helper) const {

    boost::shared_ptr<std::string> str = helper.content();
    if (helper.isXml()) {
        return XmlDocHelper(xmlReadMemory(str->c_str(), str->size(), "",
                                          charset_.empty() ? NULL : charset_.c_str(),
                                          XML_PARSE_DTDATTR | XML_PARSE_NOENT));
    }
    else if (helper.contentType() == "text/plain") {
        std::string res;
        res.append("<text>").append(XmlUtils::escape(*str)).append("</text>");
        return XmlDocHelper(xmlParseMemory(res.c_str(), res.size()));
    }
    else if (helper.contentType() == "text/html") {
        std::string data = XmlUtils::sanitize(*str, StringUtils::EMPTY_STRING, 0);
        return XmlDocHelper(xmlReadMemory(data.c_str(), data.size(), helper.base().c_str(),
                                          helper.charset().c_str(),
                                          XML_PARSE_DTDATTR | XML_PARSE_NOENT));
    }
    throw InvokeError("format is not recognized: " + helper.contentType(), "url", helper.url());
}

void
HttpBlock::createTagInfo(const HttpHelper &helper, boost::any &a) const {
    if (!tagged()) {
        return;
    }

    Tag tag = helper.createTag();
    a = boost::any(tag);
}

void
HttpBlock::checkTimeout(const TimeoutCounter &timer, const std::string &url) {
    if (timer.remained() > 0) {
        return;
    }
    InvokeError error("block is timed out", "url", url);
    error.add("timeout", boost::lexical_cast<std::string>(timer.timeout()));
    throw error;
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
    HttpHelper::init();
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

