#include "settings.h"

#include <exception>
#include <string>

#include "http_extension.h"
#include "http_block.h"

#include "xscript/http_helper.h"
#include "xscript/util.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif


namespace xscript {

HttpExtension::HttpExtension() {
}

HttpExtension::~HttpExtension() {
}

const char*
HttpExtension::name() const {
    return "http";
}

const char*
HttpExtension::nsref() const {
    return XmlUtils::XSCRIPT_NAMESPACE;
}

void
HttpExtension::initContext(Context *ctx) {
    (void)ctx;
}

void
HttpExtension::stopContext(Context *ctx) {
    (void)ctx;
}

void
HttpExtension::destroyContext(Context *ctx) {
    (void)ctx;
}

std::auto_ptr<Block>
HttpExtension::createBlock(Xml *owner, xmlNodePtr node) {
    return std::auto_ptr<Block>(new HttpBlock(this, owner, node));
}

void
HttpExtension::init(const Config *config) {
    (void)config;
    try {
        HttpHelper::init();
    }
    catch (const std::exception &e) {
        std::string error_msg("HttpExtension construction: caught exception: ");
        error_msg += e.what();
        terminate(1, error_msg.c_str(), true);
    }
    catch (...) {
        terminate(1, "HttpExtension construction: caught unknown exception", true);
    }
}

static ExtensionRegisterer ext_(ExtensionHolder(new HttpExtension()));

} // namespace xscript


extern "C" ExtensionInfo* get_extension_info() {
    static ExtensionInfo info = { "http", xscript::XmlUtils::XSCRIPT_NAMESPACE };
    return &info;
}

