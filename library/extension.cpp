#include "settings.h"

#include "xscript/extension.h"
#include "xscript/logger_factory.h"
#include "xscript/message_interface.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

const std::string Extension::ALLOW_EMPTY_NAMESPACE_METHOD = "EXTENSION_ALLOW_EMPTY_NAMESPACE";

Extension::Extension()
        : logger_(LoggerFactory::instance()->getDefaultLogger()) {
    assert(logger_);
}

Extension::~Extension() {
}

bool
Extension::checkScriptProperty(const char *prop, const char *value) {
    (void)prop;
    (void)value;
    return false;
}

void
Extension::ExtensionResourceTraits::destroy(Extension *ext) {
    // Acquire loader to avoid premature unload of shared library.
    boost::shared_ptr<Loader> loader = ext->loader();
    boost::checked_delete(ext);
};

bool
Extension::allowEmptyNamespace() const {
    MessageParam<const char> name_param(name());
    
    MessageParamBase* param_list[1];
    param_list[0] = &name_param;
    
    MessageParams params(1, param_list);
    MessageResult<bool> result;
  
    MessageProcessor::instance()->process(ALLOW_EMPTY_NAMESPACE_METHOD, params, result);
    return result.get();
}

namespace ExtensionHandlers {

//TODO: remove
class AllowEmptyNamespaceHandler : public MessageHandler {
    Result process(const MessageParams &params, MessageResultBase &result) {
        (void)params;
        result.set(true);
        return CONTINUE;
    }
};

struct HandlerRegisterer {
    HandlerRegisterer() {
        MessageProcessor::instance()->registerBack(Extension::ALLOW_EMPTY_NAMESPACE_METHOD,
                boost::shared_ptr<MessageHandler>(new AllowEmptyNamespaceHandler()));
    }
};

static HandlerRegisterer reg_handlers;

} // namespace ExtensionHandlers

Extension* const Extension::ExtensionResourceTraits::DEFAULT_VALUE = static_cast<Extension*>(NULL);

} // namespace xscript
