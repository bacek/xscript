#include "settings.h"

#include "xscript/message_interface.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

typedef std::list<boost::shared_ptr<MessageHandler> > HandlerList;
static std::map<std::string, HandlerList> handlers;

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
    HandlerList& handler_list = handlers[key];
    handler_list.push_front(handler);
}

void
MessageProcessor::registerBack(const std::string &key,
                               boost::shared_ptr<MessageHandler> handler) {
    HandlerList& handler_list = handlers[key];
    handler_list.push_back(handler);
}

void
MessageProcessor::process(const std::string &key,
                          const MessageParams &params,
                          MessageResultBase &result) {
    std::map<std::string, HandlerList>::iterator it = handlers.find(key);
    if (handlers.end() == it) {
        throw MessageError("Cannot find handler: " + key);
    }
    
    try {
        HandlerList& handler_list = it->second; 
        for(HandlerList::iterator hit = handler_list.begin();
            hit != handler_list.end();
            ++hit) {
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
