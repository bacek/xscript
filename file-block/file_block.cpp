#include "settings.h"

#include <sys/stat.h>

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

namespace xscript {

namespace fs = boost::filesystem;

FileBlock::FileBlock(const Extension *ext, Xml *owner, xmlNodePtr node)
    : Block(ext, owner, node), ThreadedBlock(ext, owner, node), TaggedBlock(ext, owner, node),
    method_(NULL), processXInclude_(false) {
}

FileBlock::~FileBlock() {
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
    else {
        throw std::invalid_argument("Unknown method for file-block: " + method());
    }
}

XmlDocHelper
FileBlock::call(Context *ctx, boost::any &a) throw (std::exception) {
    log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());

    const std::vector<Param*> &p = params();

    if (p.size() < 1 || p.size() > 2) {
        throw std::logic_error("bad arity");
    }

    std::string filename = p[0]->asString(ctx);
    if (filename.empty()) {
        throw InvokeError("empty path", "file", filename);
    }
    std::string file = fullName(filename);

    if (!tagged()) {
        return (this->*method_)(file, ctx);
    }

    struct stat st;
    int res = stat(file.c_str(), &st);
    if (res != 0) {
        // Return empty document if we can't stat file. It's not available anyway.
        return XmlDocHelper();
    }

    const Tag* tag = boost::any_cast<Tag>(&a);

    XmlDocHelper doc;
    bool modified;

    if (tag && tag->last_modified != Tag::UNDEFINED_TIME && st.st_mtime <= tag->last_modified) {
        // We got tag and file modification time not greater than last_modified in tag
        // Set "modified" to false and omit loading doc.
        modified = false;
    }
    else {
        modified = true;
        doc = (this->*method_)(file, ctx);
    }

    Tag local_tag(modified, st.st_mtime, Tag::UNDEFINED_TIME);
    a = boost::any(local_tag);

    return doc;
}

XmlDocHelper
FileBlock::loadFile(const std::string& file_name, Context *ctx) {
    (void)ctx;
    log()->debug("%s: loading file %s", BOOST_CURRENT_FUNCTION, file_name.c_str());

    PROFILER(log(), std::string(BOOST_CURRENT_FUNCTION) + ", " + owner()->name());

    XmlDocHelper doc(xmlReadFile(
                         file_name.c_str(),
                         NULL,
                         XML_PARSE_DTDATTR | XML_PARSE_DTDLOAD | XML_PARSE_NOENT)
                    );

    XmlUtils::throwUnless(NULL != doc.get());

    if (processXInclude_) {
        XmlUtils::throwUnless(xmlXIncludeProcessFlags(doc.get(), XML_PARSE_NOENT) >= 0);
    }
    return doc;
}

XmlDocHelper
FileBlock::invokeFile(const std::string& file_name, Context *ctx) {
    log()->debug("%s: invoking file %s", BOOST_CURRENT_FUNCTION, file_name.c_str());

    PROFILER(log(), std::string(BOOST_CURRENT_FUNCTION) + ", " + owner()->name());

    Context* tmp_ctx = ctx;
    unsigned int depth = 0;
    while (tmp_ctx) {
        if (file_name == tmp_ctx->script()->name()) {
            throw InvokeError("self-recursive invocation", "file", file_name);
        }

        ++depth;
        if (depth > FileExtension::max_invoke_depth_) {
            throw InvokeError("too much recursive invocation depth", "file", file_name);
        }

        tmp_ctx = tmp_ctx->parentContext();
    }

    boost::shared_ptr<Script> script = Script::create(file_name);
    boost::shared_ptr<Context> local_ctx(new Context(script, ctx->requestData()));
    local_ctx->parentContext(ctx);

    if (threaded() || ctx->forceNoThreaded()) {
        local_ctx->forceNoThreaded(true);
    }

    ContextStopper ctx_stopper(local_ctx);
    local_ctx->authContext(ctx->authContext());

    XmlDocHelper doc = script->invoke(local_ctx);
    XmlUtils::throwUnless(NULL != doc.get());

    return doc;
}

}

