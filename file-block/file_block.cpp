#include "settings.h"

#include <sys/stat.h>
#include <cerrno>
#include <fstream>

#include <boost/bind.hpp>
#include <boost/current_function.hpp>
#include <boost/filesystem/path.hpp>

#include <libxml/xinclude.h>

#include <xscript/context.h>
#include <xscript/logger.h>
#include <xscript/operation_mode.h>
#include <xscript/param.h>
#include <xscript/profiler.h>
#include <xscript/request_data.h>
#include <xscript/script.h>
#include <xscript/script_factory.h>
#include <xscript/string_utils.h>
#include <xscript/typed_map.h>
#include <xscript/util.h>
#include <xscript/xml.h>
#include <xscript/xml_util.h>

#include "file_block.h"
#include "file_extension.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

const std::string FileBlock::FILENAME_PARAMNAME = "FILEBLOCK_FILENAME_PARAMNAME";
const std::string FileBlock::INVOKE_SCRIPT_PARAMNAME = "FILEBLOCK_INVOKE_SCRIPT_PARAMNAME";

namespace fs = boost::filesystem;

FileBlock::FileBlock(const Extension *ext, Xml *owner, xmlNodePtr node)
    : Block(ext, owner, node), ThreadedBlock(ext, owner, node), TaggedBlock(ext, owner, node),
    method_(NULL), processXInclude_(false), ignore_not_existed_(false) {
}

FileBlock::~FileBlock() {
}

void
FileBlock::property(const char *name, const char *value) {
    if (strncasecmp(name, "ignore-not-existed", sizeof("ignore-not-existed")) == 0) {
        ignore_not_existed_ = (strncasecmp(value, "yes", sizeof("yes")) == 0);
    }
    else if (!TaggedBlock::propertyInternal(name, value)) {
        ThreadedBlock::property(name, value);
    }
}

void
FileBlock::postParse() {
    ThreadedBlock::postParse();
    TaggedBlock::postParse();

    createCanonicalMethod("file.");

    const std::string& met = method();
    if (0 == strcasecmp(met.c_str(), "include")) {
        method_ = &FileBlock::loadFile;
        processXInclude_ = true;
    }
    else if (0 == strcasecmp(met.c_str(), "load")) {
        method_ = &FileBlock::loadFile;
        processXInclude_ = false;
    }
    else if (0 == strcasecmp(met.c_str(), "loadText")) {
        method_ = &FileBlock::loadText;
    }
    else if (0 == strcasecmp(met.c_str(), "invoke")) {
        method_ = &FileBlock::invokeFile;
    }
    else if (0 == strcasecmp(met.c_str(), "test")) {
    }
    else {
        throw std::invalid_argument("Unknown method for file-block: " + met);
    }
}

bool
FileBlock::isTest() const {
    return !method_;
}

bool
FileBlock::isInvoke() const {
    return &FileBlock::invokeFile == method_;
}

XmlDocHelper
FileBlock::call(boost::shared_ptr<Context> ctx,
    boost::shared_ptr<InvokeContext> invoke_ctx) throw (std::exception) {
    
    log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());

    const std::vector<Param*> &p = params();
    unsigned int size = p.size();
    if (size == 0) {
        throwBadArityError();
    }
    
    std::string file;
    boost::function<std::string()> filename_creator = boost::bind(&FileBlock::fileName, this, ctx.get());
    if (isInvoke() && tagged()) {
        std::string param_name = FILENAME_PARAMNAME + boost::lexical_cast<std::string>(this);
        Context::MutexPtr mutex = ctx->param<Context::MutexPtr>(FileExtension::FILE_CONTEXT_MUTEX);
        file = ctx->param(param_name, filename_creator, *mutex);
    }
    else {
        file = filename_creator();
    }

    if (file.empty()) {
        if (isTest()) {
            return testFileDoc(false, file);
        }
        ignore_not_existed_ ? throw SkipResultInvokeError("empty path") :
            throw InvokeError("empty path");
    }
    
    if (remainedTime(ctx.get()) <= 0) {
        InvokeError error("block is timed out", "file", file);
        error.add("timeout", boost::lexical_cast<std::string>(ctx->timer().timeout()));
        throw error;
    }

    struct stat st;
    int res = stat(file.c_str(), &st);
    
    if (isTest()) {
        return testFileDoc(!res, file);
    }
    
    if (res != 0) {
        std::stringstream stream;
        StringUtils::report("failed to stat file: ", errno, stream);
        ignore_not_existed_ ? throw SkipResultInvokeError(stream.str(), "file", file) :
            throw InvokeError(stream.str(), "file", file);
    }
    
    if (!tagged()) {
        return invokeMethod(file, ctx, invoke_ctx);
    }

    const Tag& tag = invoke_ctx->tag();

    XmlDocHelper doc;
    bool modified = true;
    if (invoke_ctx->haveCachedCopy() && tag.last_modified != Tag::UNDEFINED_TIME && st.st_mtime == tag.last_modified) {
        // We got tag and file modification time equal than last_modified in tag
        // Set "modified" to false and omit loading doc.
        modified = false;
    }
    else {
        doc = invokeMethod(file, ctx, invoke_ctx);
    }

    Tag local_tag(modified, st.st_mtime, Tag::UNDEFINED_TIME);
    invoke_ctx->tag(local_tag);

    return doc;
}

XmlDocHelper
FileBlock::invokeMethod(const std::string &file_name,
        boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) {
    try {
        return (this->*method_)(file_name, ctx, invoke_ctx);
    }
    catch(InvokeError &e) {
        e.add("file", file_name);
        throw e;
    }
    catch(const std::exception &e) {
        throw InvokeError(e.what(), "file", file_name);
    }
}

XmlDocHelper
FileBlock::loadFile(const std::string &file_name,
        boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) {
    (void)ctx;
    (void)invoke_ctx;
    log()->debug("%s: loading file %s", BOOST_CURRENT_FUNCTION, file_name.c_str());

    PROFILER(log(), std::string(BOOST_CURRENT_FUNCTION) + ", " + owner()->name());

    XmlInfoCollector::Starter starter;
    XmlDocHelper doc(xmlReadFile(
        file_name.c_str(), NULL, XML_PARSE_DTDATTR | XML_PARSE_NOENT));

    XmlUtils::throwUnless(NULL != doc.get());

    if (processXInclude_) {
        XmlUtils::throwUnless(xmlXIncludeProcessFlags(doc.get(), XML_PARSE_NOENT) >= 0);
    }
    
    std::string error = XmlInfoCollector::getError();
    if (!error.empty()) {
        throw InvokeError(error);
    }
    
    OperationMode::processXmlError(file_name);
    
    return doc;
}

XmlDocHelper
FileBlock::loadText(const std::string &file_name,
        boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) {
    (void)ctx;
    (void)invoke_ctx;
    log()->debug("%s: loading text file %s", BOOST_CURRENT_FUNCTION, file_name.c_str());

    PROFILER(log(), std::string(BOOST_CURRENT_FUNCTION) + ", " + owner()->name());

    std::ifstream is(file_name.c_str(), std::ios::in);
    if (!is) {
        throw InvokeError("Cannot open file", "file", file_name);
    }
    is.exceptions(std::ios::badbit | std::ios::eofbit);

    std::ifstream::pos_type size = 0;
    if (!is.seekg(0, std::ios::end)) {
        throw InvokeError("Seek error", "file", file_name);
    }
    size = is.tellg();
    if (!is.seekg(0, std::ios::beg)) {
        throw InvokeError("Seek error", "file", file_name);
    }

    std::vector<char> doc_data(size);
    is.read(&doc_data[0], size);

    if (doc_data.empty()) {
        XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
        if (NULL != doc.get()) {
            XmlNodeHelper node(xmlNewDocNode(doc.get(), NULL, (const xmlChar*)"text", NULL));
            if (node.get() != NULL) {
                xmlDocSetRootElement(doc.get(), node.release());
            }
        }
        return doc;
    }
    std::string res("<text>");
    res.append(XmlUtils::escape(doc_data)).append("</text>");
    return XmlDocHelper(xmlReadMemory(
        res.c_str(), res.size(), "", NULL, XML_PARSE_DTDATTR | XML_PARSE_NOENT));
}

XmlDocHelper
FileBlock::invokeFile(const std::string &file_name,
        boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) {
    log()->debug("%s: invoking file %s", BOOST_CURRENT_FUNCTION, file_name.c_str());

    PROFILER(log(), std::string(BOOST_CURRENT_FUNCTION) + ", " + owner()->name());

    Context* tmp_ctx = ctx.get();
    unsigned int depth = 0;
    while (tmp_ctx) {
        if (file_name == tmp_ctx->script()->name()) {
            throw InvokeError("self-recursive invocation");
        }

        ++depth;
        if (depth > FileExtension::max_invoke_depth_) {
            throw InvokeError("too much recursive invocation depth");
        }

        tmp_ctx = tmp_ctx->parentContext();
    }

    boost::function<boost::shared_ptr<Script>()> script_creator =
        boost::bind(&ScriptFactory::createScript, file_name);

    boost::shared_ptr<Script> script;
    if (tagged()) {
        std::string script_param_name = INVOKE_SCRIPT_PARAMNAME + boost::lexical_cast<std::string>(this);
        Context::MutexPtr mutex = ctx->param<Context::MutexPtr>(FileExtension::FILE_CONTEXT_MUTEX);
        script = ctx->param(script_param_name, script_creator, *mutex);
    }
    else {
        script = script_creator();
    }
    
    if (NULL == script.get()) {
        throw InvokeError("Cannot create script", "file", file_name);
    }
    boost::shared_ptr<Context> local_ctx =
        Context::createChildContext(script, ctx, TypedMap(), true);

    if (threaded() || ctx->forceNoThreaded()) {
        local_ctx->forceNoThreaded(true);
    }

    ContextStopper ctx_stopper(local_ctx);
    
    XmlDocSharedHelper doc = script->invoke(local_ctx);
    XmlUtils::throwUnless(NULL != doc->get());
    
    if (local_ctx->noCache()) {
        invoke_ctx->resultType(InvokeContext::NO_CACHE);
    }
    
    return *doc;
}


XmlDocHelper
FileBlock::testFileDoc(bool result, const std::string &file) {
    XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != doc.get());
    
    std::string res = boost::lexical_cast<std::string>(result);
    XmlNodeHelper node(xmlNewDocNode(doc.get(), NULL, (const xmlChar*)"exist", (const xmlChar*)res.c_str()));
    XmlUtils::throwUnless(NULL != node.get());
    if (!file.empty()) {
        xmlNewProp(node.get(), (const xmlChar*)"file", (const xmlChar*)XmlUtils::escape(file).c_str());
    }

    xmlDocSetRootElement(doc.get(), node.release());
    return doc;
}

std::string
FileBlock::fileName(const Context *ctx) const {
    std::string filename = concatParams(ctx, 0, params().size() - 1);
    return filename.empty() ? filename : fullName(filename);
}

std::string
FileBlock::createTagKey(const Context *ctx) const {
    
    std::string key = processMainKey(ctx);
    boost::function<std::string()> filename_creator =
        boost::bind(&FileBlock::fileName, this, ctx);
    Context::MutexPtr mutex = ctx->param<Context::MutexPtr>(FileExtension::FILE_CONTEXT_MUTEX);
    std::string param_name = FILENAME_PARAMNAME + boost::lexical_cast<std::string>(this);
    std::string filename =
        const_cast<Context*>(ctx)->param(param_name, filename_creator, *mutex);  

    if (filename.empty()) {
        return key;
    }

    key.push_back('|');
    key.append(filename);

    if (!isInvoke()) {
        return key;
    }

    boost::function<boost::shared_ptr<Script>()> script_creator =
        boost::bind(&ScriptFactory::createScript, filename);

    std::string script_param_name = INVOKE_SCRIPT_PARAMNAME + boost::lexical_cast<std::string>(this);
    boost::shared_ptr<Script> script =
        const_cast<Context*>(ctx)->param(script_param_name, script_creator, *mutex);
        
    if (NULL == script.get()) {
        return key;
    }

    key.append(". Script: ").append(script->createTagKey(ctx, false));
    return key;
}

bool
FileBlock::allowDistributed() const {
    if (!TaggedBlock::allowDistributed()) {
        return false;
    }
    
    if (checkStrategy(LOCAL)) {
        return isInvoke();
    }
    
    return true;
}

}

