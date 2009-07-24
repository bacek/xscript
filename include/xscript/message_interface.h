#ifndef _XSCRIPT_MESSAGE_INTERFACE_H_
#define _XSCRIPT_MESSAGE_INTERFACE_H_

#include <list>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include "xscript/util.h"

namespace xscript {

class MessageError : public CriticalInvokeError {
public:
    MessageError(const std::string &error) : CriticalInvokeError(error) {}
};

class MessageParamBase {
public:
    virtual ~MessageParamBase() {};
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
    Type* getParam(unsigned int n) const {
        if (n >= size_) {
            throw MessageError("Message interface error. Argument not found: " + boost::lexical_cast<std::string>(n));
        }
        try {
            MessageParam<Type>& casted_param = dynamic_cast<MessageParam<Type>&>(*(params_[n]));
            return casted_param.value();
        }
        catch(std::bad_cast) {
            throw MessageError("Message interface error. Cannot cast argument: " + boost::lexical_cast<std::string>(n));
        }
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
        try {
            MessageResult<Type>& res = dynamic_cast<MessageResult<Type>&>(*this);
            return res.get();
        }
        catch(std::bad_cast) {
            throw MessageError("Message interface error. Cannot cast result");
        }
    }
    
    template <typename Type>
    void set(const Type &value) {
        try {
            MessageResult<Type>& res = dynamic_cast<MessageResult<Type>&>(*this);
            res.set(value);
        }
        catch(std::bad_cast) {
            throw MessageError("Message interface error. Cannot cast result");
        }
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
    
private:
    typedef std::list<boost::shared_ptr<MessageHandler> > HandlerList;
    std::map<std::string, HandlerList> handlers_;
};

} // namespace xscript

#endif // _XSCRIPT_MESSAGE_INTERFACE_H_
