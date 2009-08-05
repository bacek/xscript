// #include "config.h"

#include <libxslt/xsltutils.h>
#include <libxslt/extensions.h>
#include <libxml/xpathInternals.h>

#include "xscript/context.h"
#include "xscript/logger.h"
#include "xscript/stylesheet.h"
#include "xscript/xml_util.h"
#include "xscript/xslt_extension.h"

#include "pcre/pcre++.h"

#include <iostream>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

class XscriptRegexExtensions {
public:
    XscriptRegexExtensions();
private:
	static const char REGEX_NAMESPACE[];
};

const char XscriptRegexExtensions::REGEX_NAMESPACE[] = "http://exslt.org/regular-expressions";

static XscriptRegexExtensions xscriptRegexExtensions;

extern "C" void
xscriptRegexTest(xmlXPathParserContextPtr ctxt, int nargs) {

	if (NULL == ctxt) {
		return;
	}

	XsltParamFetcher params(ctxt, nargs);

	if (nargs < 2 || nargs > 3) {
        XmlUtils::reportXsltError("regex:test: bad param count", ctxt);
		return;
	}

	const char* match = params.str(0);
	const char* regex = params.str(1);

	std::string flags;
	if (nargs == 3) {
		const char* flag_param = params.str(2);
		if (NULL == flag_param) {
            XmlUtils::reportXsltError("regex:test: bad parameter", ctxt);
			xmlXPathReturnEmptyNodeSet(ctxt);
			return;
		}
		flags.assign(flag_param);
	}

	if (NULL == regex || NULL == match || !*regex) {
        XmlUtils::reportXsltError("regex:test: bad parameter", ctxt);
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	if (!*match) {
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	try {
		pcrepp::Pcre re(regex, flags);
		int ret = re.search(match);
		xmlXPathReturnBoolean(ctxt, ret);
	}
	catch(const std::exception& e) {
        XmlUtils::reportXsltError("regex:test: caught exception: " + std::string(e.what()), ctxt);
		ctxt->error = XPATH_EXPR_ERROR;
		xmlXPathReturnEmptyNodeSet(ctxt);
	}
	catch(...) {
        XmlUtils::reportXsltError("regex:test: caught unknown exception", ctxt);
		ctxt->error = XPATH_EXPR_ERROR;
		xmlXPathReturnEmptyNodeSet(ctxt);
	}
}

extern "C" void
xscriptRegexMatch(xmlXPathParserContextPtr ctxt, int nargs) {

	if (NULL == ctxt) {
		return;
	}
	
	XsltParamFetcher params(ctxt, nargs);

	if (nargs < 2 || nargs > 3) {
        XmlUtils::reportXsltError("regex:match: bad param count", ctxt);
		return;
	}

	const char* match = params.str(0);
	const char* regex = params.str(1);

	std::string flags;
	if (nargs == 3) {
		const char* flag_param = params.str(2);
		if (NULL == flag_param) {
            XmlUtils::reportXsltError("regex:match: bad parameter", ctxt);
			xmlXPathReturnEmptyNodeSet(ctxt);
			return;
		}
		flags.assign(flag_param);
	}

	if (NULL == regex || NULL == match || !*regex) {
        XmlUtils::reportXsltError("regex:match: bad parameter", ctxt);
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	if (!*match) {
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	xsltTransformContextPtr tctx = xsltXPathGetTransformContext(ctxt);
	if (NULL == tctx) {
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	try {
		pcrepp::Pcre re(regex, flags);
		xmlNodeSetPtr ret = xmlXPathNodeSetCreate(NULL);

        Context* ctx = Stylesheet::getContext(tctx).get();
		for(int offset = 0; re.search(match, offset); offset = re.get_match_end() + 1) {
			int matches = re.matches();
			if (matches <= 0) {
				break;
			}
			for(int i = 0; i < matches; ++i) {
				xmlNodePtr node = xmlNewText((xmlChar *)(re.get_match(i).c_str()));
				ctx->addNode(node);
				xmlXPathNodeSetAdd(ret, node);
			}
		}
		xmlXPathReturnNodeSet(ctxt, ret);
	}
    catch(const std::exception& e) {
        XmlUtils::reportXsltError("regex:match: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
    catch(...) {
        XmlUtils::reportXsltError("regex:match: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
}

extern "C" void
xscriptRegexReplace(xmlXPathParserContextPtr ctxt, int nargs) {

	if (NULL == ctxt) {
		return;
	}

	XsltParamFetcher params(ctxt, nargs);

	if (nargs != 4) {
        XmlUtils::reportXsltError("regex:replace: bad param count", ctxt);
		return;
	}

	const char* match = params.str(0);
	const char* regex = params.str(1);
	const char* flags = params.str(2);
	const char* needle = params.str(3);
	
	if (NULL == needle || NULL == flags || NULL == regex || NULL == match ||
		!*regex) {
        XmlUtils::reportXsltError("regex:replace: bad parameter", ctxt);
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	if (!*match) {
		xmlXPathReturnEmptyNodeSet(ctxt);
		return;
	}

	try {
		pcrepp::Pcre re(regex, flags);
		std::string res = re.replace(match, needle);
		valuePush(ctxt, xmlXPathNewCString(res.c_str()));
	}
    catch(const std::exception& e) {
        XmlUtils::reportXsltError("regex:replace: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
    catch(...) {
        XmlUtils::reportXsltError("regex:replace: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
}

XscriptRegexExtensions::XscriptRegexExtensions() {
	XsltFunctionRegisterer("test", REGEX_NAMESPACE, &xscriptRegexTest);
	XsltFunctionRegisterer("match", REGEX_NAMESPACE, &xscriptRegexMatch);
	XsltFunctionRegisterer("replace", REGEX_NAMESPACE, &xscriptRegexReplace);
}

} // namespace xscript
