#ifndef _XSCRIPT_MESSAGE_INTERFACE_H_
#define _XSCRIPT_MESSAGE_INTERFACE_H_

#include <cstring>
#include <list>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include <typeinfo>

#include <boost/shared_ptr.hpp>

#include "xscript/message_errors.h"

namespace xscript {

class MessageParamBase {
public:
    virtual ~MessageParamBase() {}
protected:
    MessageParamBase() {}
};

template <typename Type>
class MessageParam : public MessageParamBase {
public:
    MessageParam(Type *value) : value_(value) {}
    ~MessageParam() {}
    
    Type* value() const {
        return value_;
    }
private:
    Type* value_;
};

class MessageParams {
public:
    MessageParams();
    MessageParams(unsigned int size, MessageParamBase** params);
    ~MessageParams();
    
    template <typename Type>
    Type* getPtr(unsigned int n) const {
        if (n >= size_) {
            throw MessageParamNotFoundError(n);
        }
        if (strcmp(typeid(*(params_[n])).name(), typeid(MessageParam<Type>).name()) == 0) {
            MessageParam<Type>* casted_param = static_cast<MessageParam<Type>*>(params_[n]);
            return casted_param->value();
        }
        throw MessageParamCastError(n);
    }
    
    template <typename Type>
    Type& get(unsigned int n) const {
        Type* param = getPtr<Type>(n);
        if (NULL == param) {
            throw MessageParamNilError(n);
        }
        return *param;
    }
    
    unsigned int size() const;
private:
    unsigned int size_;
    MessageParamBase** params_;
};

template <typename Type>
class MessageResult;

class MessageResultBase {
public:
    MessageResultBase();
    virtual ~MessageResultBase();
        
    template <typename Type>
    Type& get() {
        if (strcmp(typeid(*this).name(), typeid(MessageResult<Type>).name()) == 0) {
            MessageResult<Type>* res = static_cast<MessageResult<Type>*>(this);
            return res->get();
        }
        throw MessageResultCastError();
    }
    
    template <typename Type>
    void set(const Type &value) {
        if (strcmp(typeid(*this).name(), typeid(MessageResult<Type>).name()) == 0) {
            MessageResult<Type>* res = static_cast<MessageResult<Type>*>(this);
            return res->set(value);
        }
        throw MessageResultCastError();
    }
};

template <typename Type>
class MessageResult : public MessageResultBase {
public:
    MessageResult() {}
    MessageResult(const Type &val) : result_(val)
    {}
    ~MessageResult() {}
    
    void set(const Type &val) {
        result_ = val;
    }
    
    Type& get() {
        return result_;
    }
private:
    Type result_;
};

class MessageHandler {
public:
    MessageHandler();
    virtual ~MessageHandler();

    virtual int process(const MessageParams &params, MessageResultBase &result);
};

class MessageProcessor {
public:
    ~MessageProcessor();

    static MessageProcessor* instance();
    
    void registerFront(const std::string &key, boost::shared_ptr<MessageHandler> handler);
    void registerBack(const std::string &key, boost::shared_ptr<MessageHandler> handler);
    
    void process(const std::string &key, const MessageParams &params, MessageResultBase &result);
private:
    MessageProcessor();
};

} // namespace xscript

#endif // _XSCRIPT_MESSAGE_INTERFACE_H_
