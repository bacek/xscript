#include "settings.h"

#include <strings.h>

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

bool HttpExtension::checked_headers_ = true;
bool HttpExtension::checked_query_params_ = true;
bool HttpExtension::load_entities_ = true;
bool HttpExtension::keep_alive_ = true;

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
        config->addForbiddenKey("/xscript/http-block");
        checked_headers_ = 0 != config->as<unsigned int>("/xscript/http-block/checked-headers", 1);
        checked_query_params_ = 0 != config->as<unsigned int>("/xscript/http-block/checked-query-params", 1);

        std::string value = config->as<std::string>("/xscript/http-block/load-entities", "yes"); // TODO: default "no"
        load_entities_ = !strcasecmp(value.c_str(), "yes");

        value = config->as<std::string>("/xscript/http-block/keep-alive", "no");
        keep_alive_ = !strcasecmp(value.c_str(), "yes");
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

