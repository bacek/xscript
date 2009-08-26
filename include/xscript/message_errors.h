#ifndef _XSCRIPT_MESSAGE_ERRORS_H_
#define _XSCRIPT_MESSAGE_ERRORS_H_

#include "xscript/exception.h"

namespace xscript {

class MessageError : public CriticalInvokeError {
public:
    MessageError(const std::string &error);
};

class MessageParamError : public MessageError {
public:
    MessageParamError(const std::string &error, unsigned int n);
};

class MessageParamNotFoundError : public MessageParamError {
public:
    MessageParamNotFoundError(unsigned int n);
};

class MessageParamCastError : public MessageParamError {
public:
    MessageParamCastError(unsigned int n);
};

class MessageParamNilError : public MessageParamError {
public:
    MessageParamNilError(unsigned int n);
};

class MessageResultCastError : public MessageError {
public:
    MessageResultCastError();
};

} // namespace xscript

#endif // _XSCRIPT_MESSAGE_ERRORS_H_
