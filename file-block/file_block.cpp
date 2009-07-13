#include "settings.h"

#include <sys/stat.h>
#include <cerrno>

#include <boost/bind.hpp>
#include <boost/current_function.hpp>
#include <boost/filesystem/path.hpp>

#include <xscript/context.h>
#include <xscript/operation_mode.h>
#include <xscript/logger.h>
#include <xscript/request_data.h>
#include <xscript/script.h>
#include <xscript/util.h>
#include <xscript/xml.h>
#include <xscript/param.h>
#include <xscript/profiler.h>
#include <xscript/xml_util.h>
#include <libxml/xinclude.h>
#include "file_block.h"
#include "file_extension.h"
#include <xscript/string_utils.h>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

std::string FileBlock::INVOKE_FILENAME_PARAMNAME = "FILE_INVOKE_FILENAME_PARAMNAME";
std::string FileBlock::INVOKE_SCRIPT_PARAMNAME = "FILE_INVOKE_SCRIPT_PARAMNAME";

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

    if (method() == "include") {
        method_ = &FileBlock::loadFile;
        processXInclude_ = true;
    }
    else if (method() == "load") {
        method_ = &FileBlock::loadFile;
        processXInclude_ = false;
    }
    else if (method() == "invoke") {
        method_ = &FileBlock::invokeFile;
    }
    else if (method() == "test") {
    }
    else {
        throw std::invalid_argument("Unknown method for file-block: " + method());
    }
}

bool
FileBlock::isTest() const {
    return NULL == method_;
}

bool
FileBlock::isInvoke() const {
    return &FileBlock::invokeFile == method_;
}

XmlDocHelper
FileBlock::call(boost::shared_ptr<Context> ctx, boost::any &a) throw (std::exception) {
    log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());

    const std::vector<Param*> &p = params();
    unsigned int size = p.size();
    if (size == 0) {
        throwBadArityError();
    }
    
    boost::function<std::string()> filename_creator = boost::bind(&FileBlock::fileName, this, ctx.get());
    std::string file = isInvoke() ?
            ctx->param(INVOKE_FILENAME_PARAMNAME, filename_creator) :
            filename_creator();

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
        return invokeMethod(file, ctx);
    }

    const Tag* tag = boost::any_cast<Tag>(&a);

    XmlDocHelper doc;
    bool modified = true;
    if (tag && tag->last_modified != Tag::UNDEFINED_TIME && st.st_mtime == tag->last_modified) {
        // We got tag and file modification time equal than last_modified in tag
        // Set "modified" to false and omit loading doc.
        modified = false;
    }
    else {
        doc = invokeMethod(file, ctx);
    }

    Tag local_tag(modified, st.st_mtime, Tag::UNDEFINED_TIME);
    a = boost::any(local_tag);

    return doc;
}

XmlDocHelper
FileBlock::invokeMethod(const std::string &file_name, boost::shared_ptr<Context> ctx) {
    try {
        return (this->*method_)(file_name, ctx);
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
FileBlock::loadFile(const std::string &file_name, boost::shared_ptr<Context> ctx) {
    (void)ctx;
    log()->debug("%s: loading file %s", BOOST_CURRENT_FUNCTION, file_name.c_str());

    PROFILER(log(), std::string(BOOST_CURRENT_FUNCTION) + ", " + owner()->name());

    {
        XmlInfoCollector::Starter starter;
        XmlDocHelper doc(xmlReadFile(
                             file_name.c_str(),
                             NULL,
                             XML_PARSE_DTDATTR | XML_PARSE_NOENT)
                        );
    
        XmlUtils::throwUnless(NULL != doc.get());
    
        if (processXInclude_) {
            XmlUtils::throwUnless(xmlXIncludeProcessFlags(doc.get(), XML_PARSE_NOENT) >= 0);
        }
        
        std::string error = XmlInfoCollector::getError();
        if (!error.empty()) {
            throw InvokeError(error);
        }
        return doc;
    }
}

XmlDocHelper
FileBlock::invokeFile(const std::string &file_name, boost::shared_ptr<Context> ctx) {
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

    boost::function<boost::shared_ptr<Script>()> script_creator = boost::bind(&Script::create, file_name);
    boost::shared_ptr<Script> script = ctx->param(INVOKE_SCRIPT_PARAMNAME, script_creator);
    
    if (NULL == script.get()) {
        throw InvokeError("Cannot create script", "file", file_name);
    }
    boost::shared_ptr<Context> local_ctx = Context::createChildContext(script, ctx);

    if (threaded() || ctx->forceNoThreaded()) {
        local_ctx->forceNoThreaded(true);
    }

    ContextStopper ctx_stopper(local_ctx);
    
    XmlDocHelper doc = script->invoke(local_ctx);
    XmlUtils::throwUnless(NULL != doc.get());
    
    return doc;
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
    std::string filename =
        const_cast<Context*>(ctx)->param(INVOKE_FILENAME_PARAMNAME, filename_creator);  
        
    if (filename.empty()) {
        return key;
    }

    key.append(1, ':').append(filename);

    if (!isInvoke()) {
        return key;
    }

    boost::function<boost::shared_ptr<Script>()> script_creator =
        boost::bind(&Script::create, filename);
    boost::shared_ptr<Script> script =
        const_cast<Context*>(ctx)->param(INVOKE_SCRIPT_PARAMNAME, script_creator);
        
    if (NULL == script.get()) {
        return key;
    }
    
    key.append(". Script: ").append(script->createTagKey(ctx, false));
    return key;
}

}

