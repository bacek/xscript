#include "settings.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

#include <boost/checked_delete.hpp>
#include <boost/current_function.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>

#include "xscript/block.h"
#include "xscript/context.h"
#include "xscript/exception.h"
#include "xscript/extension.h"
#include "xscript/logger.h"
#include "xscript/message_interface.h"
#include "xscript/object.h"
#include "xscript/operation_mode.h"
#include "xscript/param.h"
#include "xscript/policy.h"
#include "xscript/profiler.h"
#include "xscript/server.h"
#include "xscript/stylesheet.h"
#include "xscript/stylesheet_factory.h"
#include "xscript/xml_util.h"
#include "xscript/xslt_extension.h"
#include "xscript/xslt_profiler.h"
#include "xscript/vhost_data.h"

#include "internal/extension_list.h"
#include "internal/profiler.h"

#include <libxslt/attributes.h>
#include <libxslt/extensions.h>
#include <libxslt/imports.h>
#include <libxslt/transform.h>
#include <libxslt/variables.h>
#include <libxslt/xsltutils.h>

#include <libxml/uri.h>
#include <libxml/xinclude.h>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

struct ContextDataHelper {
    ContextDataHelper();
    boost::shared_ptr<Context> ctx;
    Stylesheet *stylesheet;
    const Block *block;
};

ContextDataHelper::ContextDataHelper() : stylesheet(NULL), block(NULL) {
}

class XsltInitalizer {
public:
    XsltInitalizer();

private:
    static void* ExtraDataInit(xsltTransformContextPtr, const xmlChar*);
    static void ExtraDataShutdown(xsltTransformContextPtr, const xmlChar*, void *data);
};

static XsltInitalizer xsltInitalizer;

const std::string Stylesheet::DETECT_OUTPUT_METHOD = "STYLESHEET_DETECT_OUTPUT_METHOD";

class Stylesheet::StylesheetData {
public:
    StylesheetData(Stylesheet *owner);
    ~StylesheetData();
    
    Block* block(xmlNodePtr node);
    void detectOutputMethod();
    void detectOutputEncoding();
    void detectOutputInfo();
    void setContentType(const char *type);
    void parseStylesheet(xsltStylesheetPtr style);    
    void parseNode(xmlNodePtr node);
    std::string detectContentType(const XmlDocHelper &doc) const;
    void appendXsltParams(const std::vector<Param*>& params,
                          const Context *ctx,
                          const InvokeContext *invoke_ctx,
                          xsltTransformContextPtr tctx);
    void attachContextData(xsltTransformContextPtr tctx,
                           boost::shared_ptr<Context> ctx,
                           Stylesheet *stylesheet,
                           const Block *block);
    
    Stylesheet *owner_;
    XsltStylesheetHelper stylesheet_;
    std::map<xmlNodePtr, Block*> blocks_;
    std::string content_type_;
    std::string output_method_;
    std::string output_encoding_;
    int omitXmlDeclaration;
    int indent;
};

Stylesheet::StylesheetData::StylesheetData(Stylesheet *owner) :
        owner_(owner), stylesheet_(NULL), blocks_(), omitXmlDeclaration(-1), indent(-1)
{}
    
Stylesheet::StylesheetData::~StylesheetData() {
    for(std::map<xmlNodePtr, Block*>::iterator i = blocks_.begin(), end = blocks_.end();
        i != end;
        ++i) {
        delete i->second;
    }
}

Block*
Stylesheet::StylesheetData::block(xmlNodePtr node) {
    std::map<xmlNodePtr, Block*>::iterator i = blocks_.find(node);
    if (i != blocks_.end()) {
        return i->second;
    }
    return NULL;
}

void
Stylesheet::StylesheetData::detectOutputMethod() {
    if (NULL == stylesheet_->method || !stylesheet_->method[0]) {
        setContentType("text/html");
        output_method_.assign("html");

        stylesheet_->methodURI = NULL;
        stylesheet_->method = xmlStrdup((const xmlChar*)"html");
    }
    else if (strncmp("xml", (const char*)stylesheet_->method, sizeof("xml")) == 0) {
        setContentType("text/xml");
    }
    else if (strncmp("html", (const char*)stylesheet_->method, sizeof("html")) == 0) {
        setContentType("text/html");
    }
    else if (strncmp("text", (const char*)stylesheet_->method, sizeof("text")) == 0) {
        setContentType("text/plain");
    }
    else {
        MessageParam<Stylesheet> stylesheet_param(owner_);

        MessageParamBase* param_list[1];
        param_list[0] = &stylesheet_param;
		    
        MessageParams params(1, param_list);
        MessageResult<std::string> result;

        MessageProcessor::instance()->process(DETECT_OUTPUT_METHOD, params, result);
        output_method_.swap(result.get());
    }
    if (output_method_.empty() && NULL != stylesheet_->method) {
        output_method_.assign((const char*)stylesheet_->method);
    }
}

void
Stylesheet::StylesheetData::detectOutputEncoding() {

    if (NULL != stylesheet_->encoding && stylesheet_->encoding[0]) {
        output_encoding_.assign((const char*)stylesheet_->encoding);
        return;
    }

    for (xsltStylesheetPtr cur = stylesheet_->imports; NULL != cur; cur = xsltNextImport(cur)) {
        if (NULL != cur->encoding && cur->encoding[0]) {
            output_encoding_.assign((const char*)cur->encoding);
            break;
        }
    }
    if (output_encoding_.empty()) {
        output_encoding_.assign(Policy::instance()->getOutputEncoding(NULL));
    }

    if (NULL != stylesheet_->encoding) {
        xmlFree(stylesheet_->encoding);
    }
    stylesheet_->encoding = xmlStrdup((const xmlChar*)(output_encoding_.c_str()));
}

void
Stylesheet::StylesheetData::detectOutputInfo() {

    xsltStylesheetPtr s = stylesheet_.get();
    XSLT_GET_IMPORT_INT(omitXmlDeclaration, s, omitXmlDeclaration);
    XSLT_GET_IMPORT_INT(indent, s, indent);
    log()->debug("%s, detectOutputInfo omitXmlDeclaration: %d, indent: %d",
	owner_->name().c_str(), omitXmlDeclaration, indent);
}

void
Stylesheet::StylesheetData::setContentType(const char *type) {
    if (content_type_.empty()) {
        content_type_.assign(type);
    }
}

void
Stylesheet::StylesheetData::parseStylesheet(xsltStylesheetPtr style) {
    for ( ; style; style = style->next) {
        if (style->doc) {
            parseNode(xmlDocGetRootElement(style->doc));
        }
        for (xsltDocumentPtr include = style->docList; include; include = include->next) {
            if (include->doc) {
                parseNode(xmlDocGetRootElement(include->doc));
            }
        }
        parseStylesheet(style->imports);
    }
}

void
Stylesheet::StylesheetData::parseNode(xmlNodePtr node) {
    ExtensionList* elist = ExtensionList::instance();
    for ( ; node ; node = node->next) {
        if (XML_ELEMENT_NODE == node->type) {
            Block *prev_block = block(node);
            if (NULL != prev_block) {
                log()->debug("%s, block %s already created (node %p)", owner_->name().c_str(), prev_block->name(), node);
                continue;
            }
            Extension *ext = elist->extension(node, false);
            if (NULL != ext) {
                log()->debug("%s, creating block %s (node %p)", owner_->name().c_str(), ext->name(), node);
                std::auto_ptr<Block> b = ext->createBlock(owner_, node);

                assert(b.get());
                b->parse();

                blocks_[node] = b.get();
                b.release();
            }
            else if (node->children) {
                parseNode(node->children);
            }
        }
    }
}

std::string
Stylesheet::StylesheetData::detectContentType(const XmlDocHelper &doc) const {
    XmlXPathContextHelper ctx(xmlXPathNewContext(doc.get()));
    XmlUtils::throwUnless(NULL != ctx.get());

    XmlXPathObjectHelper object(
        xmlXPathEvalExpression((const xmlChar*) "//*[local-name()='output']/@content-type", ctx.get()));
    XmlUtils::throwUnless(NULL != object.get());

    if (NULL != object->nodesetval && 0 != object->nodesetval->nodeNr) {
        const char *val = XmlUtils::value(object->nodesetval->nodeTab[0]);
        if (NULL != val) {
            return val;
        }
    }
    return StringUtils::EMPTY_STRING;
}

void
Stylesheet::StylesheetData::appendXsltParams(const std::vector<Param*>& params,
                                             const Context *ctx,
                                             const InvokeContext *invoke_ctx,
                                             xsltTransformContextPtr tctx) {
    if (params.empty()) {
        return;
    }

    log()->debug("param list contains %llu elements", static_cast<unsigned long long>(params.size()));

    typedef std::set<std::string> ParamSetType;
    ParamSetType unique_params;

    std::vector<std::string>::const_iterator val_iter;
    if (invoke_ctx) {
        if (invoke_ctx->xsltParams().size() != params.size()) {
            throw std::logic_error("Incorrect xslt arg list");
        }
        val_iter = invoke_ctx->xsltParams().begin();
    }
    for (std::vector<Param*>::const_iterator it = params.begin(), end = params.end();
        it != end;
        ++it) {
        const Param *param = *it;
        const std::string &id = param->id();
        std::pair<ParamSetType::iterator, bool> result = unique_params.insert(id);
        if (result.second == false) {
            std::stringstream stream;
            stream << "duplicated xslt-param: " << id << ". Url: " << ctx->request()->getOriginalUrl();
            OperationMode::instance()->processError(stream.str());
        }
        else {
            std::string value = invoke_ctx ? *val_iter : param->asString(ctx);
            if (!value.empty()) {
                log()->debug("add xslt-param %s: %s", id.c_str(), value.c_str());
                XmlUtils::throwUnless(xsltQuoteOneUserParam(
                        tctx, (const xmlChar*)id.c_str(), (const xmlChar*)value.c_str()) == 0);
            }
            else {
                log()->debug("skip empty xslt-param: %s", id.c_str());
            }
        }
        if (invoke_ctx) {
            ++val_iter;
        }
    }
}

void
Stylesheet::StylesheetData::attachContextData(xsltTransformContextPtr tctx,
                                              boost::shared_ptr<Context> ctx,
                                              Stylesheet *stylesheet,
                                              const Block *block) {
    ContextDataHelper* data =
        static_cast<ContextDataHelper*>(xsltGetExtData(tctx, (const xmlChar*)XmlUtils::XSCRIPT_NAMESPACE));
    XmlUtils::throwUnless(NULL != data);
    data->ctx = ctx;
    data->stylesheet = stylesheet;
    data->block = block;
}


Stylesheet::Stylesheet(const std::string &name) :
    Xml(name), data_(new StylesheetData(this)) {
}

Stylesheet::~Stylesheet() {
}

void
Stylesheet::setContentType(const char *type) {
    data_->setContentType(type);
}

xsltStylesheetPtr
Stylesheet::stylesheet() const {
    return data_->stylesheet_.get();
}

const std::string&
Stylesheet::contentType() const {
    return data_->content_type_;
}

const std::string&
Stylesheet::outputMethod() const {
    return data_->output_method_;
}

const std::string&
Stylesheet::outputEncoding() const {
    return data_->output_encoding_;
}

bool
Stylesheet::omitXmlDecl() const {
    return data_->omitXmlDeclaration > 0;
}

bool
Stylesheet::indent() const {
    return data_->indent > 0;
}

XmlDocHelper
Stylesheet::apply(Object *obj, boost::shared_ptr<Context> ctx,
        boost::shared_ptr<InvokeContext> invoke_ctx, xmlDocPtr doc) {

    log()->entering("Stylesheet::apply");
    
    XsltTransformContextHelper tctx(xsltNewTransformContext(data_->stylesheet_.get(), doc));
    XmlUtils::throwUnless(NULL != tctx.get());

    log()->debug("%s: transform context created", name().c_str());

    data_->attachContextData(tctx.get(), ctx, this, dynamic_cast<const Block*>(obj));

    const Server* server = VirtualHostData::instance()->getServer();
    bool use_profile = server ? server->useXsltProfiler() : false;

    tctx->profile = use_profile;
    if (NULL == tctx->globalVars) {
        tctx->globalVars = xmlHashCreate(20);
    }

    if (obj) {
        data_->appendXsltParams(obj->xsltParams(), ctx.get(), invoke_ctx.get(), tctx.get());
    }

    internal::Profiler profiler("Total apply time");

    XmlDocHelper newdoc(
            xsltApplyStylesheetUser(data_->stylesheet_.get(), doc,
                                NULL, NULL, NULL, tctx.get()));

    // Looks like we have to do something with this.
    if (use_profile && obj) {
        XmlDocHelper prof_doc(xsltGetProfileInformation(tctx.get()));
        xmlNewTextChild(xmlDocGetRootElement(prof_doc.get()), 0,
                        (const xmlChar*) "total-time",
                        (const xmlChar*) profiler.getInfo().c_str());
        
        XsltProfiler::instance()->insertProfileDoc(name(), prof_doc.release());
    }

    log()->debug("%s: %s, checking result document", BOOST_CURRENT_FUNCTION, name().c_str());
    XmlUtils::throwUnless(NULL != newdoc.get());
    return newdoc;
}

void
Stylesheet::parse() {

    namespace fs = boost::filesystem;

    fs::path path(name());
    std::string file_str;

#ifdef BOOST_FILESYSTEM_VERSION
    #if BOOST_FILESYSTEM_VERSION == 3
        file_str = path.native();
    #else
        file_str = path.native_file_string();
    #endif
#else
    file_str = path.native_file_string();
#endif

    if (!fs::exists(path) || fs::is_directory(path)) {
        throw CanNotOpenError(file_str);
    }

    PROFILER(log(), "Stylesheet.parse " + file_str);
    XmlCharHelper canonic_path(xmlCanonicPath((const xmlChar *)file_str.c_str()));

    {
        XmlInfoCollector::Starter starter;
        
        XmlDocHelper doc(xmlReadFile((const char*) canonic_path.get(), NULL,
            XML_PARSE_DTDATTR | XML_PARSE_NOENT | XML_PARSE_NOCDATA));

        XmlUtils::throwUnless(NULL != doc.get());
        log()->debug("%s stylesheet %s document parsed", BOOST_CURRENT_FUNCTION, name().c_str());

        if (NULL == doc->children) {
            throw std::runtime_error("got empty xml doc");
        }

        XmlUtils::throwUnless(xmlXIncludeProcessFlags(doc.get(), XML_PARSE_NOENT) >= 0);

        if (NULL == xmlDocGetRootElement(doc.get())) {
            throw std::runtime_error("got xml doc with no root");
        }
        
        std::string type = data_->detectContentType(doc);
        if (!type.empty()) {
            data_->content_type_ = type;
        }

        data_->stylesheet_ = XsltStylesheetHelper(xsltParseStylesheetDoc(doc.get()));
        XmlUtils::throwUnless(NULL != data_->stylesheet_.get());
        doc.release();
        
        TimeMapType* modified_info = XmlInfoCollector::getModifiedInfo();
        TimeMapType fake;
        modified_info ? swapModifiedInfo(*modified_info) : swapModifiedInfo(fake);
        
        std::string error = XmlInfoCollector::getError();
        if (!error.empty()) {
            throw UnboundRuntimeError(error);
        }
        
        OperationMode::instance()->processXmlError(name());
    }

    data_->parseStylesheet(data_->stylesheet_.get());
    
    data_->detectOutputMethod();
    data_->detectOutputEncoding();
    data_->detectOutputInfo();
    
    XsltTransformContextHelper tctx(
        xsltNewTransformContext(data_->stylesheet_.get(), XmlUtils::fakeXml()));
    
    XmlUtils::throwUnless(NULL != tctx.get());
}

boost::shared_ptr<Context>
Stylesheet::getContext(xsltTransformContextPtr tctx) {
    ContextDataHelper* data =
        static_cast<ContextDataHelper*>(xsltGetExtData(tctx, (const xmlChar*) XmlUtils::XSCRIPT_NAMESPACE));
    XmlUtils::throwUnless(NULL != data);
    return data->ctx;
}

Stylesheet*
Stylesheet::getStylesheet(xsltTransformContextPtr tctx) {
    ContextDataHelper* data =
        static_cast<ContextDataHelper*>(xsltGetExtData(tctx, (const xmlChar*) XmlUtils::XSCRIPT_NAMESPACE));
    XmlUtils::throwUnless(NULL != data);
    return data->stylesheet;
}

const Block*
Stylesheet::getBlock(xsltTransformContextPtr tctx) {
    ContextDataHelper* data =
        static_cast<ContextDataHelper*>(xsltGetExtData(tctx, (const xmlChar*) XmlUtils::XSCRIPT_NAMESPACE));
    XmlUtils::throwUnless(NULL != data);
    return data->block;
}

Block*
Stylesheet::block(xmlNodePtr node) {
    return data_->block(node);
}


class StylesheetDetectOutputMethodHandler : public MessageHandler {
    Result process(const MessageParams &params, MessageResultBase &result) {

        Stylesheet *style = params.getPtr<Stylesheet>(0);
        xsltStylesheetPtr st = style->stylesheet();

        if (NULL == st->methodURI || !st->methodURI[0] ||
            NULL == st->method || !st->method[0] ||
            0 != strcasecmp(XmlUtils::XSCRIPT_NAMESPACE, (const char*)st->methodURI)) {
            return CONTINUE;
        }

        if (!strncmp("xhtml", (const char*)st->method, sizeof("xhtml"))) {
            style->setContentType("text/html");

            xmlFree(st->method);
            xmlFree(st->methodURI);

            st->methodURI = NULL;
            st->method = xmlStrdup((const xmlChar*)"xml");

            result.set<std::string>("xhtml");
            return BREAK;
        }
        if (!strncmp("wml", (const char*)st->method, sizeof("wml"))) {
            style->setContentType("text/vnd.wap.wml");

            xmlFree(st->method);
            xmlFree(st->methodURI);

            st->methodURI = NULL;
            st->omitXmlDeclaration = 0;
            st->method = xmlStrdup((const xmlChar*)"xml");

            result.set<std::string>("wml");
            return BREAK;
        }
        return CONTINUE;
    }
};
				
class StylesheetHandlersRegisterer {
public:
    StylesheetHandlersRegisterer() {
        MessageProcessor::instance()->registerBack(Stylesheet::DETECT_OUTPUT_METHOD,
            boost::shared_ptr<MessageHandler>(new StylesheetDetectOutputMethodHandler()));
    }
};


XsltInitalizer::XsltInitalizer()  {
    xsltRegisterExtModule((const xmlChar*) XmlUtils::XSCRIPT_NAMESPACE, ExtraDataInit, ExtraDataShutdown);
}

void*
XsltInitalizer::ExtraDataInit(xsltTransformContextPtr, const xmlChar*) {

    void* data = new ContextDataHelper();
    log()->debug("%s, data is: %p", BOOST_CURRENT_FUNCTION, data);
    return data;
}

void
XsltInitalizer::ExtraDataShutdown(xsltTransformContextPtr, const xmlChar*, void* data) {

    log()->debug("%s, data is: %p", BOOST_CURRENT_FUNCTION, data);
    delete static_cast<ContextDataHelper*>(data);
}

static StylesheetHandlersRegisterer reg_stylesheet_handlers;

} // namespace xscript
