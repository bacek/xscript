#include "settings.h"
#include "xscript/validator_exception.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

ValidatorException::ValidatorException()
    : InvokeError("validation failed") {
}

ValidatorException::ValidatorException(const std::string& reason)
    : InvokeError("validation failed: " + reason) {
}

} // namespace xscript
