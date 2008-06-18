#include "settings.h"

#include <sstream>
#include <algorithm>
#include <exception>
#include <boost/lexical_cast.hpp>
#include <boost/current_function.hpp>

#include <libxslt/xsltutils.h>
#include <libxslt/transform.h>
#include <libxslt/extensions.h>

#include <libxml/xpathInternals.h>

#include "xscript/util.h"
#include "xscript/state.h"
#include "xscript/range.h"
#include "xscript/block.h"
#include "xscript/logger.h"
#include "xscript/context.h"
#include "xscript/encoder.h"
#include "xscript/response.h"
#include "xscript/stylesheet.h"
#include "xscript/xml_helpers.h"
#include "xscript/xslt_extension.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

class XsltExtensions
{
public:
	XsltExtensions();
};

static XsltExtensions xsltExtensions;

extern "C" void
xscriptXsltHttpHeaderOut(xmlXPathParserContextPtr ctxt, int nargs) {

	log()->entering("xscript:http-header-out");
	if (NULL == ctxt) {
		return;
	}
	if (2 != nargs) {
		log()->error("bad param count in xscript:http-header-out");
		return;
	}

	XsltParamFetcher params(ctxt, nargs);

	const char* name = params.str(0);
	const char* value = params.str(1);
	
	if (NULL == name || NULL == value) {
		log()->error("bad parameter in xscript:http-header-out");
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}
	
	xsltTransformContextPtr tctx = xsltXPathGetTransformContext(ctxt);
	if (NULL == tctx) {
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}
	
	try {
		Context *ctx = Stylesheet::getContext(tctx);
		Response *response = ctx->response();
		response->setHeader(name, value);
	}
	catch (const std::exception &e) {
		log()->error("caught exception in [xscript:http-header-out]: %s", e.what());
		ctxt->error = XPATH_EXPR_ERROR;
	}
	xmlXPathReturnEmptyNodeSet(ctxt);
}

extern "C" void
xscriptXsltHttpRedirect(xmlXPathParserContextPtr ctxt, int nargs) {
	
	log()->entering("xscript:http-redirect");
	if (ctxt == NULL) {
		return;
	}
	if (1 != nargs) {
		log()->error("xscript:http-redirect: bad param count");
		return;
	}

	XsltParamFetcher params(ctxt, nargs);
	
	const char* str = params.str(0);
	if (NULL == str) {
		log()->error("xscript:http-redirect: bad parameter");
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	xsltTransformContextPtr tctx = xsltXPathGetTransformContext(ctxt);
	if (NULL == tctx) {
		
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	try {
		Context* ctx = Stylesheet::getContext(tctx);
		Response* response = ctx->response();
		response->redirectToPath(str);
	}
	catch (const std::exception &e) {
		log()->error("caught exception in [xscript:http-redirect]: %s", e.what());
		ctxt->error = XPATH_EXPR_ERROR;
	}
	xmlXPathReturnEmptyNodeSet(ctxt);
}

extern "C" void
xscriptXsltSetHttpStatus(xmlXPathParserContextPtr ctxt, int nargs) {

	log()->entering("xscript:set-http-status");
	if (ctxt == NULL) {
		return;
	}
	if (1 != nargs) {
		log()->error("xscript:set-http-status: bad param count");
		return;
	}

	XsltParamFetcher params(ctxt, nargs);

	const char* str = params.str(0);
	if (NULL == str) {
		log()->error("xscript:set-http-status: bad parameter");
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}
	
	xsltTransformContextPtr tctx = xsltXPathGetTransformContext(ctxt);
	if (NULL == tctx) {
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	try {
		unsigned short status = boost::lexical_cast<unsigned short>(str);
		Context* ctx = Stylesheet::getContext(tctx);
		Response *response = ctx->response();
		response->setStatus(status);
		xmlXPathReturnNumber(ctxt, status);
	}
	catch (const std::exception &e) {
		log()->error("%s, caught exception: %s", BOOST_CURRENT_FUNCTION, e.what());
		ctxt->error = XPATH_EXPR_ERROR;
		xmlXPathReturnEmptyNodeSet(ctxt);
	}
}

extern "C" void
xscriptXsltGetStateArg(xmlXPathParserContextPtr ctxt, int nargs) {

	log()->entering("xscript:get-state-arg");
	if (ctxt == NULL) {
		return;
	}
	if (1 != nargs) {
		log()->error("xscript:get-state-arg: bad param count");
		return;
	}

	XsltParamFetcher params(ctxt, nargs);
	
	const char* str = params.str(0);
	if (NULL == str) {
		log()->error("xscript:get-state-arg: bad parameter");
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	xsltTransformContextPtr tctx = xsltXPathGetTransformContext(ctxt);
	if (NULL == tctx) {
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	try {
		Context *ctx = Stylesheet::getContext(tctx);
		boost::shared_ptr<State> state = ctx->state();
		valuePush(ctxt, xmlXPathNewCString(state->asString(str).c_str()));
	}
	catch (const std::exception &e) {
		log()->error("caught exception in [xscript:get-state-arg]: %s", e.what());
		ctxt->error = XPATH_EXPR_ERROR;
		xmlXPathReturnEmptyNodeSet(ctxt);
	}
}

extern "C" void
xscriptXsltSanitize(xmlXPathParserContextPtr ctxt, int nargs) {

	log()->entering("xscript:sanitize");
	if (ctxt == NULL) {
		return;
	}
	if (1 != nargs) {
		log()->error("xscript:sanitize: bad param count");
		return;
	}

	XsltParamFetcher params(ctxt, nargs);
	
	const char* str = params.str(0);
	if (NULL == str) {
		log()->error("xscript:sanitize: bad parameter");
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	xsltTransformContextPtr tctx = xsltXPathGetTransformContext(ctxt);
	if (NULL == tctx) {
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	try {
		std::stringstream stream;
		stream << "<sanitized>" << XmlUtils::sanitize(str) << "</sanitized>" << std::endl;
		std::string str = stream.str();

		XmlDocHelper doc(xmlParseMemory(str.c_str(), str.length()));
		XmlUtils::throwUnless(NULL != doc.get());
		
		xmlNodePtr node = xmlCopyNode(xmlDocGetRootElement(doc.get()), 1);

		Context* ctx = Stylesheet::getContext(tctx);
		ctx->addNode(node);

		xmlNodeSetPtr ret = xmlXPathNodeSetCreate(node);
		xmlXPathReturnNodeSet(ctxt, ret);
	}
	catch (const std::exception &e) {
		log()->error("caught exception in [xscript::sanitize]: %s", e.what());
		ctxt->error = XPATH_EXPR_ERROR;
		xmlXPathReturnEmptyNodeSet(ctxt);
	}
}

extern "C" void
xscriptXsltUrlencode(xmlXPathParserContextPtr ctxt, int nargs) {

	log()->entering("xscript:urlencode");
	if (ctxt == NULL) {
		return;
	}
	if (2 != nargs) {
		log()->error("xscript:urlencode: bad param count");
		return;
	}

	XsltParamFetcher params(ctxt, nargs);

	const char* value = params.str(1);
	const char* encoding = params.str(0);
	if (NULL == value || NULL == encoding) {
		log()->error("xscript:urlencode: bad parameter");
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	try {
		std::auto_ptr<Encoder> encoder = Encoder::createEscaping("utf-8", encoding);
		std::string encoded;
		encoder->encode(createRange(value), encoded);
		valuePush(ctxt, xmlXPathNewCString(StringUtils::urlencode(encoded).c_str()));
	}
	catch (const std::exception &e) {
		log()->error("caught exception in [xscript:urlencode]: %s", e.what());
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
	if (2 != nargs) {
		log()->error("xscript:urldecode: bad param count");
		return;
	}

	XsltParamFetcher params(ctxt, nargs);

	const char* value = params.str(1);
	const char* encoding = params.str(0);
	
	if (NULL == value || NULL == encoding) {
		log()->error("xscript:urldecode: bad parameter");
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	try {
		std::auto_ptr<Encoder> encoder = Encoder::createEscaping(encoding, "utf-8");
		
		std::string decoded;
		encoder->encode(StringUtils::urldecode(value), decoded);
		valuePush(ctxt, xmlXPathNewCString(decoded.c_str()));
	}
	catch (const std::exception &e) {
		log()->error("caught exception in [xscript:urldecode]: %s", e.what());
		ctxt->error = XPATH_EXPR_ERROR;
		xmlXPathReturnEmptyNodeSet(ctxt);
	}
}


void
xscriptTemplatedEscStr(const char *str, const char *esc_template, std::string &result) {

	log()->debug("templatedEscStr: %s", str);

	std::string s(str);
	std::string::size_type pos = (std::string::size_type)-1;
	while (true) {
		pos = s.find("\r\n", pos + 1);
		if (pos == std::string::npos) {
			break;
		}
		s.erase(pos, 1);
	}

	for (std::string::const_iterator i = s.begin(), end = s.end(); i != end; ++i) {
		char ch = *i;
		if (ch == '\r' || ch == '\n') {
			result.append(1, '\\');
			ch = 'n';
		}
		else {
			const char *res = strchr(esc_template, ch);
			if (NULL != res) {
				result.append(1, '\\');
			}
		}
		result.append(1, ch);
	}
}

void
xscriptXsltEscStr(const char *str, std::string &result) {

	log()->debug("xscriptXsltEscStr: %s", str);
	xscriptTemplatedEscStr(str, "\\/-'\"", result);
}

void
xscriptXsltJSQuoteStr(const char *str, std::string &result) {

	log()->debug("xscriptXsltJSQuoteStr: %s", str);
	xscriptTemplatedEscStr(str, "\\/-'", result);
}

extern "C" void
xscriptXsltEsc(xmlXPathParserContextPtr ctxt, int nargs) {

	log()->entering("xscript:esc");
	if (ctxt == NULL) {
		return;
	}
	if (1 != nargs) {
		log()->error("xscript:esc: bad param count");
		return;
	}
	
	XsltParamFetcher params(ctxt, nargs);

	const char* str = params.str(0);
	if (NULL == str) {
		log()->error("xscript:esc: bad parameter");
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	try {
		std::string result;
		xscriptXsltEscStr(str, result);
		valuePush(ctxt, xmlXPathNewCString(result.c_str()));
	}
	catch (const std::exception &e) {
		log()->error("caught exception in [xscript:esc]: %s", e.what());
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
	if (1 != nargs) {
		log()->error("xscript:js-quote: bad param count");
		return;
	}

	XsltParamFetcher params(ctxt, nargs);
	const char* str = params.str(0);

	if (NULL == str) {
		log()->error("xscript:js-quote: bad parameter");
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	try {
		std::string result;
		result.append(1,'\'');
		xscriptXsltJSQuoteStr(str, result);
		result.append(1, '\'');
		valuePush(ctxt, xmlXPathNewCString(result.c_str()));
	}
	catch (const std::exception &e) {
		log()->error("caught exception in [xscript:js-quote]: %s", e.what());
		ctxt->error = XPATH_EXPR_ERROR;
		xmlXPathReturnEmptyNodeSet(ctxt);
	}
}

extern "C" void
xscriptXsltMD5(xmlXPathParserContextPtr ctxt, int nargs) {

	log()->entering("xscript:md5");
	if (ctxt == NULL) {
		return;
	}
	if (1 != nargs) {
		log()->error("xscript:md5: bad param count");
		return;
	}

	XsltParamFetcher params(ctxt, nargs);

	const char* str = params.str(0);
	if (NULL == str) {
		log()->error("xscript:md5: bad parameter");
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	try {
		std::string result = HashUtils::hexMD5(str);
		valuePush(ctxt, xmlXPathNewCString(result.c_str()));
	}
	catch (const std::exception &e) {
		log()->error("caught exception in [xscript:md5]: %s", e.what());
		ctxt->error = XPATH_EXPR_ERROR;
		xmlXPathReturnEmptyNodeSet(ctxt);
	}
}

extern "C" void
xscriptXsltWbr(xmlXPathParserContextPtr ctxt, int nargs) {

	log()->entering("xscript:wbr");
	if (ctxt == NULL) {
		return;
	}

	if (2 != nargs) {
		log()->error("xscript:wbr: bad param count");
		return;
	}

	XsltParamFetcher params(ctxt, nargs);

	const char* str = params.str(0);
	if (NULL == str) {
		log()->error("xscript:wbr: bad first parameter");
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	if (*str == '\0') {
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	const char* str2 = params.str(1);
	if (NULL == str2) {
		log()->error("xscript:wbr: bad second parameter");
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	long int length;
	try {
		length = boost::lexical_cast<long int>(str2);
	}
	catch(const boost::bad_lexical_cast&) {
		log()->error("xscript:wbr: incorrect length format");
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	if (length <= 0) {
		log()->error("xscript:wbr: incorrect length value");
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	xsltTransformContextPtr tctx = xsltXPathGetTransformContext(ctxt);
	if (NULL == tctx) {
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	try {
		Context* ctx = Stylesheet::getContext(tctx);
		XmlNodeSetHelper ret(xmlXPathNodeSetCreate(NULL));

		const char* end = str + strlen(str);
		const char* chunk_begin = str;
		if (end - str > length)  {
			long int char_passed = 0;
			while (str < end) {
				if (isspace(*str)) {
					char_passed = 0;
				}
				else {
					if (char_passed == length) {
						xmlNodePtr node = xmlNewTextLen((xmlChar*)chunk_begin, str - chunk_begin);
						xmlXPathNodeSetAdd(ret.get(), node);
						ctx->addNode(node);

						node = xmlNewNode(NULL, (xmlChar*)"wbr");
						xmlXPathNodeSetAdd(ret.get(), node);
						ctx->addNode(node);

						chunk_begin = str;
						char_passed = 0;
					}
					++char_passed;
				}
				str = StringUtils::nextUTF8(str);
			}

			if (str > end) {
				throw std::runtime_error("incorrect UTF8 data");
			}
		}

		xmlNodePtr node = xmlNewTextLen((xmlChar*)chunk_begin, end - chunk_begin);
		xmlXPathNodeSetAdd(ret.get(), node);
		ctx->addNode(node);

		xmlXPathReturnNodeSet(ctxt, ret.release());
	}
	catch (const std::exception &e) {
		log()->error("caught exception in [xscript::wbr]: %s", e.what());
		ctxt->error = XPATH_EXPR_ERROR;
		xmlXPathReturnEmptyNodeSet(ctxt);
	}
}

extern "C" void
xscriptExtElementBlock(xsltTransformContextPtr tctx, xmlNodePtr node, xmlNodePtr inst, xsltElemPreCompPtr comp) {
	(void)comp;
	if (tctx == NULL) {
		log()->error("%s: no transformation context", BOOST_CURRENT_FUNCTION);
		return;
	}
	if (node == NULL) {
		log()->error("%s: no current node", BOOST_CURRENT_FUNCTION);
		return;
	}
	if (inst == NULL) {
		log()->error("%s: no instruction", BOOST_CURRENT_FUNCTION);
		return;
	}
	if (tctx->insert == NULL) {
		log()->error("%s: no insertion point", BOOST_CURRENT_FUNCTION);
		return;
	}
	Context *ctx = Stylesheet::getContext(tctx);
	Stylesheet *stylesheet = Stylesheet::getStylesheet(tctx);
	if (NULL == ctx || NULL == stylesheet) {
		log()->error("%s: no context or stylesheet", BOOST_CURRENT_FUNCTION);
		return;
	}
	Block *block = stylesheet->block(inst);
	if (NULL == block) {
		log()->error("%s: no block found (inst %p)", BOOST_CURRENT_FUNCTION, inst);
		return;
	}
	try {
		XmlDocHelper doc = block->invoke(ctx);
		if (NULL != doc.get()) {
			xmlAddChild(tctx->insert, xmlCopyNode(xmlDocGetRootElement(doc.get()), 1));
			return;
		}
		log()->error("%s: empty result", BOOST_CURRENT_FUNCTION);
	}
	catch (const std::exception &e) {
		log()->error("%s, caught exception: %s", BOOST_CURRENT_FUNCTION, e.what());
		XmlDocHelper doc = block->errorResult(e.what());
		if (NULL != doc.get()) {
			xmlAddChild(tctx->insert, xmlCopyNode(xmlDocGetRootElement(doc.get()), 1));
		}
	}
}

XsltFunctionRegisterer::XsltFunctionRegisterer(const char *name, const char *nsref, xmlXPathFunction func) {
	xsltRegisterExtModuleFunction((const xmlChar*) name, (const xmlChar*) nsref, func);
}

XsltElementRegisterer::XsltElementRegisterer(const char* name, const char* nsref, xsltTransformFunction func) {
	xsltRegisterExtModuleElement((const xmlChar*) name, (const xmlChar*) nsref, NULL, func);
}

void
XsltElementRegisterer::registerBlockInvokation(const char *name, const char *nsref) {
	xsltRegisterExtModuleElement((const xmlChar*) name, (const xmlChar*) nsref, NULL, xscriptExtElementBlock);
}

XsltParamFetcher::XsltParamFetcher(xmlXPathParserContextPtr ctxt, int nargs) {
	try {
		strings_.reserve(nargs);
		for (int i = 0; i < nargs; ++i) {
			strings_.push_back(xmlXPathPopString(ctxt));
		}
		std::reverse(strings_.begin(), strings_.end());
	}
	catch (const std::exception &e) {
		clear();
		throw;
	}
}

XsltParamFetcher::~XsltParamFetcher() {
	clear();
}

unsigned int
XsltParamFetcher::size() const {
	return static_cast<unsigned int>(strings_.size());
}

const char*
XsltParamFetcher::str(unsigned int index) const {
	return (const char*) strings_.at(index);
}

void
XsltParamFetcher::clear() {
	for (std::vector<xmlChar*>::iterator i = strings_.begin(), end = strings_.end(); i != end; ++i) {
		xmlFree(*i);
	}
}

XsltExtensions::XsltExtensions() {

	XsltFunctionRegisterer("esc", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltEsc);
	XsltFunctionRegisterer("js-quote", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltJsQuote);
	XsltFunctionRegisterer("md5", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltMD5);
	XsltFunctionRegisterer("wbr", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltWbr);

	XsltFunctionRegisterer("sanitize", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltSanitize);
	
	XsltFunctionRegisterer("urlencode", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltUrlencode);
	XsltFunctionRegisterer("urldecode", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltUrldecode);

	XsltFunctionRegisterer("http-redirect", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltHttpRedirect);
	XsltFunctionRegisterer("http_redirect", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltHttpRedirect);
	
	XsltFunctionRegisterer("get-state-arg", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltGetStateArg);
	XsltFunctionRegisterer("get_state_arg", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltGetStateArg);

	XsltFunctionRegisterer("http-header-out", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltHttpHeaderOut);
	XsltFunctionRegisterer("http_header_out", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltHttpHeaderOut);
	
	XsltFunctionRegisterer("set-http-status", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltSetHttpStatus);
	XsltFunctionRegisterer("set_http_status", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltSetHttpStatus);
}

} // namespace xscript
