#include "settings.h"

#include <sys/stat.h>
#include <cerrno>
#include <fstream>

#include <boost/bind.hpp>
#include <boost/filesystem/path.hpp>

#include <libxml/xinclude.h>

#include <xscript/args.h>
#include <xscript/context.h>
#include <xscript/logger.h>
#include <xscript/operation_mode.h>
#include <xscript/profiler.h>
#include <xscript/request_data.h>
#include <xscript/script.h>
#include <xscript/script_factory.h>
#include <xscript/string_utils.h>
#include <xscript/typed_map.h>
#include <xscript/util.h>
#include <xscript/writer.h>
#include <xscript/xml.h>
#include <xscript/xml_util.h>
#include <xscript/json2xml.h>

#include "file_block.h"
#include "file_extension.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

static const std::string FILENAME_PARAMNAME = "FILEBLOCK_FILENAME_PARAMNAME";
static const std::string INVOKE_SCRIPT_PARAMNAME = "FILEBLOCK_INVOKE_SCRIPT_PARAMNAME";
static const std::string STR_FILE = "file";
static const std::string STR_TIMEOUT = "timeout";
static const std::string STR_EMPTY_PATH = "empty path";


enum { GLB_MUTEX_SIZE = 57 };
static boost::mutex global_mutex_[GLB_MUTEX_SIZE];

inline static boost::mutex&
getGlobalMutex(const void *ptr) {
    size_t index = (reinterpret_cast<size_t>(ptr)) % GLB_MUTEX_SIZE;
    return global_mutex_[index];
}


namespace fs = boost::filesystem;

class FStreamBinaryWriter : public BinaryWriter {
public:
	FStreamBinaryWriter(boost::shared_ptr<std::ifstream> is, std::streamsize size) :
		is_(is), size_(size)
	{}

    void write(std::ostream *os, const Response *response) const {
        (void)response;
        *os << is_->rdbuf();
    }

    std::streamsize size() const {
        return size_;
    }
    
    virtual ~FStreamBinaryWriter() {}
private:
    boost::shared_ptr<std::ifstream> is_;
    std::streamsize size_;
};

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
    else if (0 == strcasecmp(met.c_str(), "loadBinary")) {
        method_ = &FileBlock::loadBinary;
    }
    else if (0 == strcasecmp(met.c_str(), "invoke")) {
        method_ = &FileBlock::invokeFile;
    }
    else if (0 == strcasecmp(met.c_str(), "test")) {
    }
    else if (0 == strcasecmp(met.c_str(), "loadJson") && Json2Xml::instance()->enabled()) {
        method_ = &FileBlock::loadJson;
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

std::string
FileBlock::fileName(InvokeContext *invoke_ctx) const {
    assert(invoke_ctx);
    const std::string &fname = invoke_ctx->extraKey(FILENAME_PARAMNAME);
    if (!fname.empty()) {
        return fname;
    }

    const ArgList *args = invoke_ctx->getArgList();
    unsigned int size = args->size();
    if (size == 0) {
        throwBadArityError();
    }
    
    std::string filename = Block::concatArguments(args, 0, size - 1);
    if (!filename.empty()) {
        filename = fullName(filename);
        invoke_ctx->extraKey(FILENAME_PARAMNAME, filename);
    }
    return filename;
}

static XmlDocSharedHelper
testFileDoc(const std::string &file, const struct stat *st) {
    XmlDocSharedHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != doc.get());
    
    XmlNodeHelper node(xmlNewDocNode(doc.get(), NULL, (const xmlChar*)"exist", (const xmlChar*)(NULL != st ? "1" : "0")));
    XmlUtils::throwUnless(NULL != node.get());
    if (!file.empty()) {
        xmlNewProp(node.get(), (const xmlChar*)STR_FILE.c_str(), (const xmlChar*)XmlUtils::escape(file).c_str());
        if (NULL != st) {
            xmlNewProp(node.get(), (const xmlChar*)"mtime",
                (const xmlChar*)boost::lexical_cast<std::string>(st->st_mtime).c_str());
        }
    }

    xmlDocSetRootElement(doc.get(), node.release());
    return doc;
}


void
FileBlock::call(boost::shared_ptr<Context> ctx,
    boost::shared_ptr<InvokeContext> invoke_ctx) const throw (std::exception) {
    
    std::string file = fileName(invoke_ctx.get());
    if (log()->enabledDebug()) {
        log()->debug("FileBlock.%s %s %s", method().c_str(), file.c_str(), owner()->name().c_str());
    }

    if (file.empty()) {
        if (isTest()) {
            invoke_ctx->resultDoc(testFileDoc(file, NULL));
            return;
        }
        ignore_not_existed_ ? throw SkipResultInvokeError(STR_EMPTY_PATH) :
            throw InvokeError(STR_EMPTY_PATH);
    }
    
    if (remainedTime(ctx.get()) <= 0) {
        InvokeError error("block is timed out", STR_FILE, file);
        error.add(STR_TIMEOUT, boost::lexical_cast<std::string>(ctx->timer().timeout()));
        throw error;
    }

    struct stat st;
    int res = stat(file.c_str(), &st);
    
    if (isTest()) {
        invoke_ctx->resultDoc(testFileDoc(file, !res ? &st : NULL));
        return;
    }
    
    if (res != 0) {
        std::stringstream stream;
        StringUtils::report("failed to stat file: ", errno, stream);
        ignore_not_existed_ ? throw SkipResultInvokeError(stream.str(), STR_FILE, file) :
            throw InvokeError(stream.str(), STR_FILE, file);
    }
    
    if (!tagged()) {
        invoke_ctx->resultDoc(invokeMethod(file, ctx, invoke_ctx));
        return;
    }

    const Tag& tag = invoke_ctx->tag();

    XmlDocSharedHelper doc;
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
    invoke_ctx->resultDoc(doc);
}

XmlDocSharedHelper
FileBlock::invokeMethod(const std::string &file_name,
        boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const {
    try {
        return (this->*method_)(file_name, ctx, invoke_ctx);
    }
    catch (const CanNotOpenError &e) {
        if (ignore_not_existed_) {
            throw SkipResultInvokeError(e.whatStr());
        }
        throw;
    }
    catch (InvokeError &e) {
        e.add(STR_FILE, file_name);
        throw;
    }
    catch (const std::exception &e) {
        throw InvokeError(e.what(), STR_FILE, file_name);
    }
}

XmlDocSharedHelper
FileBlock::loadFile(const std::string &file_name,
        boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const {
    (void)ctx;
    (void)invoke_ctx;

    PROFILER(log(), "file.load: " + file_name + " , " + owner()->name());

    XmlInfoCollector::Starter starter;
    XmlDocSharedHelper doc(xmlReadFile(
        file_name.c_str(), NULL, XML_PARSE_DTDATTR | XML_PARSE_NOENT));

    XmlUtils::throwUnless(NULL != doc.get());

    if (processXInclude_) {
        XmlUtils::throwUnless(xmlXIncludeProcessFlags(doc.get(), XML_PARSE_NOENT) >= 0);
    }
    
    std::string error = XmlInfoCollector::getError();
    if (!error.empty()) {
        throw InvokeError(error);
    }
    
    OperationMode::instance()->processXmlError(file_name);
    
    return doc;
}

XmlDocSharedHelper
FileBlock::loadJson(const std::string &file_name,
        boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const {
    (void)ctx;
    (void)invoke_ctx;

    PROFILER(log(), "file.loadJson: " + file_name + " , " + owner()->name());

    std::ifstream is(file_name.c_str(), std::ios::in);
    if (!is) {
        throw CanNotOpenError(file_name);
    }
    //fixed crush : unexpected exception
    //is.exceptions(std::ios::badbit | std::ios::eofbit);

    XmlNodeHelper node(Json2Xml::instance()->convert(is));
    if (NULL == node.get()) {
        throw InvokeError("Cannot convert json file to xml");
    }
    XmlDocSharedHelper result(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != result.get());
        
    xmlDocSetRootElement(result.get(), node.release());
    
    return result;
}

static std::streamsize
detectStreamSize(std::ifstream &is) {
    is.exceptions(std::ios::badbit | std::ios::eofbit);

    if (is.seekg(0, std::ios::end)) {
        std::ifstream::pos_type size = is.tellg();
        if (is.seekg(0, std::ios::beg)) {
            return size;
	}
    }
    throw InvokeError("Seek error");
}

XmlDocSharedHelper
FileBlock::loadText(const std::string &file_name,
        boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const {
    (void)ctx;
    (void)invoke_ctx;

    PROFILER(log(), "file.loadText: " + file_name + " , " + owner()->name());

    std::ifstream is(file_name.c_str(), std::ios::in);
    if (!is) {
        throw CanNotOpenError(file_name);
    }

    std::streamsize size = detectStreamSize(is);

    std::vector<char> doc_data(size);
    is.read(&doc_data[0], size);
    is.close();

    XmlDocSharedHelper result(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != result.get());

    XmlNodeHelper node(xmlNewDocNode(result.get(), NULL, (const xmlChar*)"text", NULL));
    XmlUtils::throwUnless(NULL != node.get());

    if (!doc_data.empty()) {
        std::string res;
        res.reserve(doc_data.size() * 2 + 20);
        XmlUtils::escape(doc_data, res);
        xmlNodeSetContent(node.get(), (const xmlChar*) res.c_str());
    }
    xmlDocSetRootElement(result.get(), node.release());
    return result;
}

XmlDocSharedHelper
FileBlock::loadBinary(const std::string &file_name,
        boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const {
    (void)invoke_ctx;

    PROFILER(log(), "file.loadBinary: " + file_name + " , " + owner()->name());

    boost::shared_ptr<std::ifstream> is(
        new std::ifstream(file_name.c_str(), std::ios::in | std::ios::binary));
    
    if (!is.get()) {
        throw CanNotOpenError(file_name);
    }

    std::streamsize size = detectStreamSize(*is);

    ctx->response()->write(
        std::auto_ptr<BinaryWriter>(new FStreamBinaryWriter(is, size)));
    
    XmlDocSharedHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != doc.get());
    
    XmlNodeHelper node(xmlNewDocNode(doc.get(), NULL, (const xmlChar*)"success", (const xmlChar*)"1"));
    XmlUtils::throwUnless(NULL != node.get());
          
    xmlNewProp(node.get(), (const xmlChar*)STR_FILE.c_str(), (const xmlChar*)XmlUtils::escape(file_name).c_str());
    xmlDocSetRootElement(doc.get(), node.release());

    return doc;
}

XmlDocSharedHelper
FileBlock::invokeFile(const std::string &file_name,
        boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const {

    PROFILER(log(), "file.invoke: " + file_name + " , " + owner()->name());
    
    for (Context* tmp_ctx = ctx.get(); tmp_ctx; tmp_ctx = tmp_ctx->parentContext().get()) {
        if (file_name == tmp_ctx->script()->name()) {
            throw InvokeError("self-recursive invocation");
        }
    }

    boost::function<boost::shared_ptr<Script>()> script_creator =
        boost::bind(&ScriptFactory::createScript, file_name);

    boost::shared_ptr<Script> script;
    if (tagged()) {
        std::string script_param_name = INVOKE_SCRIPT_PARAMNAME + file_name;
        script = ctx->param(script_param_name, script_creator, getGlobalMutex(ctx.get()));
    }
    else {
        script = script_creator();
    }
    
    if (NULL == script.get()) {
        throw InvokeError("Cannot create script", STR_FILE, file_name);
    }

    boost::shared_ptr<TypedMap> local_params(new TypedMap());
    boost::shared_ptr<Context> local_ctx =
        Context::createChildContext(script, ctx, invoke_ctx, local_params, Context::PROXY_ALL);

    ContextStopper ctx_stopper(local_ctx);
    
    XmlDocSharedHelper doc = script->invoke(local_ctx);    
    XmlUtils::throwUnless(NULL != doc.get());
    
    if (local_ctx->noCache()) {
        invoke_ctx->resultType(InvokeContext::NO_CACHE);
    }
    
    ctx_stopper.reset();
    return doc;
}


static const std::string STR_SCRIPTNAME_DELIMITER = ". Script: ";

std::string
FileBlock::createTagKey(const Context *ctx, const InvokeContext *invoke_ctx) const {
    
    std::string key = processMainKey(ctx, invoke_ctx);
    std::string filename = fileName(const_cast<InvokeContext*>(invoke_ctx));
    if (filename.empty()) {
        return key;
    }

    bool is_invoke = isInvoke();
    key.reserve(filename.size() + (is_invoke ? 300 : 1));
    key.push_back('|');
    key.append(filename);

    if (!is_invoke) {
        return key;
    }

    boost::function<boost::shared_ptr<Script>()> script_creator =
        boost::bind(&ScriptFactory::createScript, filename);

    std::string script_param_name = INVOKE_SCRIPT_PARAMNAME + filename;

    boost::shared_ptr<Script> script;
    try {
        script = const_cast<Context*>(ctx)->param(script_param_name, script_creator, getGlobalMutex(ctx));
    }
    catch (const CanNotOpenError &e) {
	if (ignore_not_existed_) {
            throw SkipResultInvokeError(e.whatStr());
	}
	throw;
    }

    if (NULL == script.get()) {
        return key;
    }

    std::string script_key = script->createBlockTagKey(ctx);
    key.reserve(STR_SCRIPTNAME_DELIMITER.size() + script_key.size());
    key.append(STR_SCRIPTNAME_DELIMITER).append(script_key);
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

