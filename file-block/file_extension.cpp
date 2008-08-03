#include <xscript/util.h>
#include "file_extension.h"
#include "file_block.h"

namespace xscript
{

unsigned int FileExtension::max_invoke_depth_;

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
	max_invoke_depth_ = config->as<unsigned int>("/xscript/file-block/max-invoke-depth", 10);
}


static ExtensionRegisterer ext_(ExtensionHolder(new FileExtension()));

}


extern "C" ExtensionInfo* get_extension_info() {
    static ExtensionInfo info = { "file", xscript::XmlUtils::XSCRIPT_NAMESPACE };
    return &info;
}

