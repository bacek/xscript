#include "settings.h"

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
#include "xscript/xml.h"
#include "xscript/util.h"
#include "xscript/param.h"
#include "xscript/logger.h"
#include "xscript/context.h"
#include "xscript/request.h"
#include "xscript/state.h"
#include "xscript/tagged_cache.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

class HttpMethodRegistrator
{
public:
	HttpMethodRegistrator();
};

MethodMap HttpBlock::methods_;

HttpBlock::HttpBlock(const Extension *ext, Xml *owner, xmlNodePtr node) :
	Block(ext, owner, node), ThreadedBlock(ext, owner, node), TaggedBlock(ext, owner, node), proxy_(false), method_(NULL)
{
}

HttpBlock::~HttpBlock() {
}

void
HttpBlock::postParse() {
	ThreadedBlock::postParse();
	TaggedBlock::postParse();

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
		ThreadedBlock::property(name, value);
	}
}

XmlDocHelper
HttpBlock::getHttp(Context *ctx, boost::any &a) {
	
	log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());
	
	const std::vector<Param*> &p = params();

	if (p.size() < 1 || p.size() > 2) {
		throw std::logic_error("getHttp: bad arity");
	}

	std::string url = p[0]->asString(ctx);
	if (strncasecmp(url.c_str(), "file://", sizeof("file://") - 1) == 0) {
		namespace fs = boost::filesystem;
		url = url.substr(sizeof("file://") - 1);
		fs::path file_path(url);
		if (!fs::exists(file_path)) {
			throw std::runtime_error(url + " is not exist");
		}

		XmlDocHelper doc = XmlDocHelper(xmlParseFile(file_path.native_file_string().c_str()));
		if (doc.get() == NULL){
			throw std::runtime_error(std::string("Got empty document. URL: ") + url);
		}

		if (tagged()) {
			a = boost::any(Tag());
		}

		return doc;
	}

	const Tag* tag = boost::any_cast<Tag>(&a);
	HttpHelper helper(url, remoteTimeout());
	helper.appendHeaders(ctx->request(), proxy_, tag);

	helper.perform();
	log()->debug("%s, http call performed", BOOST_CURRENT_FUNCTION);
	helper.checkStatus();
	
	if (tagged()) {
		createTagInfo(helper, a);
	}
	return response(helper);
}
	
XmlDocHelper 
HttpBlock::getHttpObsolete(Context *ctx, boost::any &a)
{
    log()->warn("Obsoleted call");
    return getHttp(ctx, a);
}

XmlDocHelper
HttpBlock::postHttp(Context *ctx, boost::any &a) {

	log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());
	
	const std::vector<Param*> &p = params();

	if (p.size() < 2 || p.size() > 3) {
		throw std::logic_error("postHttp: bad arity");
	}
	
	const Tag* tag = boost::any_cast<Tag>(&a);
	HttpHelper helper(p[0]->asString(ctx), remoteTimeout());
	std::string body = p[1]->asString(ctx);
	helper.appendHeaders(ctx->request(), proxy_, tag);

	helper.postData(body.data(), body.size());
	
	helper.perform();
	helper.checkStatus();
	
	createTagInfo(helper, a);
	return response(helper);
}

XmlDocHelper
HttpBlock::getByState(Context *ctx, boost::any &a) {
	(void)a;

	log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());

	const std::vector<Param*> &p = params();

	if (p.size() != 1 || tagged()) {
		throw std::logic_error("getByState: bad arity");
	}

	std::string url = p[0]->asString(ctx);
	bool has_query = url.find('?') != std::string::npos;

	boost::shared_ptr<State> state = ctx->state();
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

	HttpHelper helper(url, remoteTimeout());
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

	if (p.size() != 1 || tagged()) {
		throw std::logic_error("getByRequest: bad arity");
	}

	std::string url = p[0]->asString(ctx);
	const std::string &query = ctx->request()->getQueryString();
	if (!query.empty()) {
		url.append(1, url.find('?') != std::string::npos ? '&' : '?');
		url.append(query);
	}

	HttpHelper helper(url, remoteTimeout());
	helper.appendHeaders(ctx->request(), proxy_, NULL);

	helper.perform();
	log()->debug("%s, http call performed", BOOST_CURRENT_FUNCTION);
	helper.checkStatus();

	return response(helper);
}

XmlDocHelper
HttpBlock::response(const HttpHelper &helper) const {
	
	const std::string& str = helper.content();
	if (helper.isXml()) {
		return XmlDocHelper(xmlReadMemory(str.c_str(), str.size(), "",
			charset_.empty() ? NULL : charset_.c_str(), XML_PARSE_DTDATTR | XML_PARSE_DTDLOAD | XML_PARSE_NOENT));
	}
	else if (helper.contentType() == "text/plain") {
		std::string res;
		res.append("<text>").append(XmlUtils::escape(str)).append("</text>");
		return XmlDocHelper(xmlParseMemory(res.c_str(), res.size()));
	}
	else if (helper.contentType() == "text/html") {
		std::string data = XmlUtils::sanitize(str);
		return XmlDocHelper(htmlReadDoc((const xmlChar*) data.c_str(), helper.base().c_str(), 
			helper.charset().c_str(), HTML_PARSE_NOBLANKS | HTML_PARSE_NONET | HTML_PARSE_NOERROR));
	}
	throw std::runtime_error("format is not recognized");
}

void
HttpBlock::createTagInfo(const HttpHelper &helper, boost::any &a) const {
	Tag tag = helper.createTag();
	a = boost::any(tag);
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

HttpMethodRegistrator::HttpMethodRegistrator() 
{
	HttpBlock::registerMethod("getHttp", &HttpBlock::getHttp);
	HttpBlock::registerMethod("get_http", &HttpBlock::getHttpObsolete);

	HttpBlock::registerMethod("getHTTP", &HttpBlock::getHttpObsolete);
	HttpBlock::registerMethod("getPageT", &HttpBlock::getHttpObsolete);
	HttpBlock::registerMethod("curlGetHttp", &HttpBlock::getHttpObsolete);
	
	HttpBlock::registerMethod("postHttp", &HttpBlock::postHttp);
	HttpBlock::registerMethod("post_http", &HttpBlock::postHttp);
	
	HttpBlock::registerMethod("getByState", &HttpBlock::getByState);
	HttpBlock::registerMethod("get_by_state", &HttpBlock::getByState);
	
	HttpBlock::registerMethod("getByRequest", &HttpBlock::getByRequest);
	HttpBlock::registerMethod("get_by_request", &HttpBlock::getByRequest);
}

static HttpMethodRegistrator reg_;
static ExtensionRegisterer ext_(ExtensionHolder(new HttpExtension()));

} // namespace xscript


extern "C" ExtensionInfo* get_extension_info() {
    static ExtensionInfo info = { "http", xscript::XmlUtils::XSCRIPT_NAMESPACE };
    return &info;
}

