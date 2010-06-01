#include "settings.h"

#include <xscript/config.h>
#include <xscript/context.h>
#include <xscript/xml_util.h>

#include "while_block.h"
#include "while_extension.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

WhileExtension::WhileExtension() {
}

WhileExtension::~WhileExtension() {
}

const char*
WhileExtension::name() const {
    return "while";
}

const char*
WhileExtension::nsref() const {
    return XmlUtils::XSCRIPT_NAMESPACE;
}

void
WhileExtension::initContext(Context *ctx) {
    (void)ctx;
}

void
WhileExtension::stopContext(Context *ctx) {
    (void)ctx;
}

void
WhileExtension::destroyContext(Context *ctx) {
    (void)ctx;
}

std::auto_ptr<Block>
WhileExtension::createBlock(Xml *owner, xmlNodePtr node) {
    return std::auto_ptr<Block>(new WhileBlock(this, owner, node));
}

void
WhileExtension::init(const Config *config) {
    (void)config;
}

bool
WhileExtension::allowEmptyNamespace() const {
    return false;
}

static ExtensionRegisterer ext_(ExtensionHolder(new WhileExtension()));

}
