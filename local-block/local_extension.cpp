#include "settings.h"

#include <xscript/config.h>
#include <xscript/context.h>
#include <xscript/message_interface.h>
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

namespace LocalExtensionHandlers {

class AllowEmptyNamespaceHandler : public MessageHandler {
    Result process(const MessageParams &params, MessageResultBase &result) {
        const char* name = params.getPtr<const char>(0);
        if (0 == strcasecmp(name, "local")) {
            result.set(false);
            return BREAK;
        }
        return CONTINUE;
    }
};

struct HandlerRegisterer {
    HandlerRegisterer() {
        MessageProcessor::instance()->registerFront(Extension::ALLOW_EMPTY_NAMESPACE_METHOD,
                boost::shared_ptr<MessageHandler>(new AllowEmptyNamespaceHandler()));
    }
};

static HandlerRegisterer reg_handlers;

} // namespace LocalExtensionHandlers

static ExtensionRegisterer ext_(ExtensionHolder(new LocalExtension()));

}

extern "C" ExtensionInfo* get_extension_info() {
    static ExtensionInfo info = { "local", xscript::XmlUtils::XSCRIPT_NAMESPACE };
    return &info;
}

