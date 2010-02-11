#include "settings.h"

#include <xscript/config.h>
#include <xscript/context.h>
#include <xscript/xml_util.h>

#include "local_block.h"
#include "local_extension.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

LocalExtension::LocalExtension() {
}

LocalExtension::~LocalExtension() {
}

const char*
LocalExtension::name() const {
    return "local";
}

const char*
LocalExtension::nsref() const {
    return XmlUtils::XSCRIPT_NAMESPACE;
}

void
LocalExtension::initContext(Context *ctx) {
    (void)ctx;
}

void
LocalExtension::stopContext(Context *ctx) {
    (void)ctx;
}

void
LocalExtension::destroyContext(Context *ctx) {
    (void)ctx;
}

std::auto_ptr<Block>
LocalExtension::createBlock(Xml *owner, xmlNodePtr node) {
    return std::auto_ptr<Block>(new LocalBlock(this, owner, node));
}

void
LocalExtension::init(const Config *config) {
    (void)config;
}

static ExtensionRegisterer ext_(ExtensionHolder(new LocalExtension()));

}

extern "C" ExtensionInfo* get_extension_info() {
    static ExtensionInfo info = { "local", xscript::XmlUtils::XSCRIPT_NAMESPACE };
    return &info;
}

