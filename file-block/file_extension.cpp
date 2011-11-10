#include "settings.h"

#include "xscript/config.h"
#include "xscript/xml_util.h"

#include "file_block.h"
#include "file_extension.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

FileExtension::FileExtension() {
}

FileExtension::~FileExtension() {
}

const char* FileExtension::name() const {
    return "file";
}

const char* FileExtension::nsref() const {
    return XmlUtils::XSCRIPT_NAMESPACE;
}

void FileExtension::initContext(Context *ctx) {
    (void)ctx;
}


void FileExtension::stopContext(Context *ctx) {
    (void)ctx;
}

void FileExtension::destroyContext(Context *ctx) {
    (void)ctx;
}

std::auto_ptr<Block> FileExtension::createBlock(Xml *owner, xmlNodePtr node) {
    return std::auto_ptr<Block>(new FileBlock(this, owner, node));
}

void FileExtension::init(const Config *config) {
    (void)config;
}


static ExtensionRegisterer ext_(ExtensionHolder(new FileExtension()));

}


extern "C" ExtensionInfo* get_extension_info() {
    static ExtensionInfo info = { "file", xscript::XmlUtils::XSCRIPT_NAMESPACE };
    return &info;
}

