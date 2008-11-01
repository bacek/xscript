#include "xscript/validator_exception.h"

namespace xscript
{

ValidatorException::ValidatorException()
    : InvokeError("validation failed") {
}

ValidatorException::ValidatorException(const std::string& reason)
    : InvokeError("validation failed: " + reason) {
}

}
