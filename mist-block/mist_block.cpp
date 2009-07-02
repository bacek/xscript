#include "settings.h"

#include "mist_block.h"

#include "xscript/param.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class MistMethodRegistrator {
public:
    MistMethodRegistrator();
};

MistBlock::MistBlock(const Extension *ext, Xml *owner, xmlNodePtr node) :
        Block(ext, owner, node), worker_(NULL) {
}

MistBlock::~MistBlock() {
}

void
MistBlock::postParse() {
    Block::postParse();
    worker_ = MistWorker::create(method());
}

XmlDocHelper
MistBlock::call(boost::shared_ptr<Context> ctx, boost::any &) throw (std::exception) {
    assert(worker_.get());
    
    XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != doc.get());
    
    std::map<unsigned int, std::string> overrides;
    if (worker_->isAttachStylesheet()) {
        const std::vector<Param*> &p = params();
        if (!p.empty()) {
            overrides.insert(std::make_pair(0, fullName(p[0]->asString(ctx.get()))));
        }
    }
    
    try {
        xmlDocSetRootElement(doc.get(), worker_->run(ctx.get(), params(), overrides).release());
    }
    catch(const std::invalid_argument &e) {
        throwBadArityError();
    }
    
    return doc;
}

MistExtension::MistExtension() {
}

MistExtension::~MistExtension() {
}

const char*
MistExtension::name() const {
    return "mist";
}

const char*
MistExtension::nsref() const {
    return XmlUtils::XSCRIPT_NAMESPACE;
}

void
MistExtension::initContext(Context *ctx) {
    (void)ctx;
}

void
MistExtension::stopContext(Context *ctx) {
    (void)ctx;
}

void
MistExtension::destroyContext(Context *ctx) {
    (void)ctx;
}

std::auto_ptr<Block>
MistExtension::createBlock(Xml *owner, xmlNodePtr node) {
    return std::auto_ptr<Block>(new MistBlock(this, owner, node));
}

void
MistExtension::init(const Config *config) {
    (void)config;
}

static ExtensionRegisterer ext_(ExtensionHolder(new MistExtension()));

} // namespace xscript


extern "C" ExtensionInfo* get_extension_info() {
    static ExtensionInfo info = { "mist", xscript::XmlUtils::XSCRIPT_NAMESPACE };
    return &info;
}

