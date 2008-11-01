#ifndef _XSCRIPT_VALIDATOR_EXCEPTION_H_
#define _XSCRIPT_VALIDATOR_EXCEPTION_H_

#include <xscript/util.h>

namespace xscript
{
    /**
     * Exception thrown on param validation error.
     */
    class ValidatorException : public InvokeError {
    public:
        ValidatorException();
        ValidatorException(const std::string& reason);
    };
}

#endif
