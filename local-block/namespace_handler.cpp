#include "settings.h"

#include <xscript/extension.h>
#include <xscript/message_interface.h>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

namespace LocalExtensionHandlers {

class AllowEmptyNamespaceHandler : public MessageHandler {
    Result process(const MessageParams &params, MessageResultBase &result) {
        const char* name = params.getPtr<const char>(0);
        if (0 == strcasecmp(name, "local") || 0 == strcasecmp(name, "while")) {
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
} // xscript

