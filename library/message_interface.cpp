#include "settings.h"

#include <stdexcept>

#include <iostream>

#include "xscript/message_interface.h"

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

MessageParams::MessageParams() :
    size_(0), params_(NULL)
{}

MessageParams::MessageParams(unsigned int size, MessageParamBase** params) :
    size_(size), params_(params)
{
    for(unsigned int i = 0; i < size; ++i) {
        if (NULL == params[i]) {
            throw MessageParamError("Null param", i);
        }
    }
}

MessageParams::~MessageParams()
{}

unsigned int
MessageParams::size() const {
    return size_;
}

MessageResultBase::MessageResultBase()
{}

MessageResultBase::~MessageResultBase()
{}

MessageHandler::MessageHandler()
{}

MessageHandler::~MessageHandler()
{}

int
MessageHandler::process(const MessageParams &params, MessageResultBase &result) {
    (void)params;
    (void)result;
    return 0;
}

MessageProcessor::MessageProcessor()
{}

MessageProcessor::~MessageProcessor()
{}

MessageProcessor*
MessageProcessor::instance() {
    static MessageProcessor processor;
    return &processor;
}

void
MessageProcessor::registerFront(const std::string &key,
                                boost::shared_ptr<MessageHandler> handler) {
    HandlerList& handlers = handlers_[key];
    handlers.push_front(handler);
}

void
MessageProcessor::registerBack(const std::string &key,
                               boost::shared_ptr<MessageHandler> handler) {
    HandlerList& handlers = handlers_[key];
    handlers.push_back(handler);
}

void
MessageProcessor::process(const std::string &key,
                          const MessageParams &params,
                          MessageResultBase &result) {
    std::map<std::string, HandlerList>::iterator it = handlers_.find(key);
    if (handlers_.end() == it) {
        throw MessageError("Message interface error. Cannot find handler: " + key);
    }
    
    try {
        HandlerList& handlers = it->second; 
        for(HandlerList::iterator hit = handlers.begin(); hit != handlers.end(); ++hit) {
            if ((*hit)->process(params, result) < 0) {
                break;
            }
        }
    }
    catch(const MessageError &e) {
        throw MessageError(std::string(e.what()) + ". Method: " + key);
    }
    
}

} // namespace xscript
