#ifndef _XSCRIPT_VALIDATOR_EXCEPTION_H_
#define _XSCRIPT_VALIDATOR_EXCEPTION_H_

#include <stdexcept>

namespace xscript
{
    /**
     * Exception thrown on param validation error.
     */
    class ValidatorException : public std::runtime_error {
    };
}

#endif
