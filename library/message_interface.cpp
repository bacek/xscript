#include "settings.h"

#include "xscript/message_interface.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

MessageParams::MessageParams() :
    size_(0), params_(NULL)
{}

MessageParams::MessageParams(unsigned int size, MessageParamBase** params) :
    size_(size), params_(params)
{
    if (size > 0 && NULL == params) {
        throw MessageError("Cannot process null message params");
    }
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
        throw MessageError("Cannot find handler: " + key);
    }
    
    try {
        HandlerList& handlers = it->second; 
        for(HandlerList::iterator hit = handlers.begin(); hit != handlers.end(); ++hit) {
            if ((*hit)->process(params, result) < 0) {
                break;
            }
        }
    }
    catch(MessageError &e) {
        e.append(". Method: " + key);
        throw e;
    }
}

} // namespace xscript
