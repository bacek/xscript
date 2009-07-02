#include "settings.h"

#include "xscript/block.h"
#include "xscript/logger.h"
#include "xscript/stylesheet.h"
#include "xscript/xml_util.h"
#include "xscript/xslt_extension.h"

#include "mist_worker.h"

#include <libxml/xpathInternals.h>
#include <libxslt/extensions.h>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class MistXsltExtensions {
public:
    MistXsltExtensions();
};

static MistXsltExtensions mistXsltExtensions;

extern "C" void
xscriptXsltMist(xmlXPathParserContextPtr ctxt, int nargs) {

    log()->entering("xscript:mist");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    const char* method = params.str(0);
    if (NULL == method || '\0' == method) {
        XmlUtils::reportXsltError("xscript:mist: bad parameter method", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }
   
    xsltTransformContextPtr tctx = xsltXPathGetTransformContext(ctxt);
    if (NULL == tctx) {
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }

    try {
        std::auto_ptr<MistWorker> worker = MistWorker::create(method);
        std::map<unsigned int, std::string> overrides;
        if (worker->isAttachStylesheet()) {
            if (params.size() > 1) {
                const char* xslt_name = params.str(1);
                if (NULL != xslt_name) {
                    Stylesheet* style = Stylesheet::getStylesheet(tctx);
                    const Block* block = Stylesheet::getBlock(tctx);
                    std::string name = block ? block->fullName(xslt_name) : style->fullName(xslt_name);
                    overrides.insert(std::make_pair(0, name));
                }
            }
        }

        boost::shared_ptr<Context> ctx = Stylesheet::getContext(tctx);
        XmlNodeHelper node = worker->run(ctx.get(), params, overrides);

        XmlNodeSetHelper ret(xmlXPathNodeSetCreate(NULL));
        xmlXPathNodeSetAdd(ret.get(), node.get());
        ctx->addNode(node.release());

        xmlXPathReturnNodeSet(ctxt, ret.release());
    }
    catch (const std::invalid_argument &e) {
        XmlUtils::reportXsltError("xscript:mist: bad param count", ctxt);
        xmlXPathReturnEmptyNodeSet(ctxt);
    }  
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:mist: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:mist: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
        xmlXPathReturnEmptyNodeSet(ctxt);
    }
}

extern "C" void
xscriptXsltSetStateString(xmlXPathParserContextPtr ctxt, int nargs) {
    
    log()->entering("xscript:set-state-string");
    if (ctxt == NULL) {
        return;
    }

    XsltParamFetcher params(ctxt, nargs);

    xsltTransformContextPtr tctx = xsltXPathGetTransformContext(ctxt);
    if (NULL == tctx) {
        xmlXPathReturnEmptyNodeSet(ctxt);
        return;
    }
    
    try {
        std::auto_ptr<MistWorker> worker = MistWorker::create("set_state_string");
        std::map<unsigned int, std::string> overrides;
        if (params.size() > 1) {
            overrides.insert(std::make_pair(0, params.str(0)));
            overrides.insert(std::make_pair(1, params.str(1)));
        }
        worker->run(Stylesheet::getContext(tctx).get(), params, overrides);
    }
    catch (const std::exception &e) {
        XmlUtils::reportXsltError("xscript:set-state-string: caught exception: " + std::string(e.what()), ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
    }
    catch (...) {
        XmlUtils::reportXsltError("xscript:set-state-string: caught unknown exception", ctxt);
        ctxt->error = XPATH_EXPR_ERROR;
    }
    
    xmlXPathReturnEmptyNodeSet(ctxt);
}

MistXsltExtensions::MistXsltExtensions() {
    XsltFunctionRegisterer("mist", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltMist);
    XsltFunctionRegisterer("set-state-string", XmlUtils::XSCRIPT_NAMESPACE, &xscriptXsltSetStateString);
}

} // namespace xscript
