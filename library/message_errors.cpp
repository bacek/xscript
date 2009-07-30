#include "settings.h"

#include "xscript/message_errors.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

MessageError::MessageError(const std::string &error) :
    CriticalInvokeError("Message interface error. " + error)
{}

MessageParamError::MessageParamError(const std::string &error, unsigned int n) :
    MessageError(error + ": " + boost::lexical_cast<std::string>(n))
{}

MessageParamNotFoundError::MessageParamNotFoundError(unsigned int n) :
    MessageParamError("Argument not found", n)
{}

MessageParamCastError::MessageParamCastError(unsigned int n) :
    MessageParamError("Cannot cast argument", n)
{}

MessageParamNilError::MessageParamNilError(unsigned int n) :
    MessageParamError("Nil argument", n)
{}

MessageResultCastError::MessageResultCastError() :
    MessageError("Cannot cast result")
{}

} // namespace xscript
