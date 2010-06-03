#include "settings.h"
#include <cstring>
#include <boost/lexical_cast.hpp>

#include "xscript/context.h"
#include "xscript/operation_mode.h"
#include "xscript/script.h"
#include "xscript/threaded_block.h"
#include "xscript/vhost_data.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

struct ThreadedBlock::ThreadedBlockData {
    ThreadedBlockData() :
        threaded_(false),
        timeout_(VirtualHostData::instance()->getConfig()->defaultBlockTimeout()),
        check_elapsed_time_(false)
    {}
    
    ~ThreadedBlockData() {
    }
    
    bool threaded_;
    int timeout_;
    bool check_elapsed_time_;
    
    static const std::string SHOW_ELAPSED_TIME;
};

const std::string ThreadedBlock::ThreadedBlockData::SHOW_ELAPSED_TIME = "SHOW_ELAPSED_TIME";

ThreadedBlock::ThreadedBlock(const Extension *ext, Xml *owner, xmlNodePtr node) :
        Block(ext, owner, node), trb_data_(new ThreadedBlockData()) {
}

ThreadedBlock::~ThreadedBlock() {
}

int
ThreadedBlock::timeout() const {
    return trb_data_->timeout_;
}

int
ThreadedBlock::invokeTimeout() const {
    return timeout();
}

bool
ThreadedBlock::threaded() const {
    return trb_data_->threaded_;
}

void
ThreadedBlock::threaded(bool value) {
    trb_data_->threaded_ = value;
}

void
ThreadedBlock::property(const char *name, const char *value) {
    if (strncasecmp(name, "threaded", sizeof("threaded")) == 0) {
        trb_data_->threaded_ = (strncasecmp(value, "yes", sizeof("yes")) == 0);
    }
    else if (strncasecmp(name, "timeout", sizeof("timeout")) == 0) {
        try {
            trb_data_->timeout_ = boost::lexical_cast<int>(value);
            if (trb_data_->timeout_ <= 0) {
                throw std::runtime_error("Positive timeout allowed only");
            }
        }
        catch(const boost::bad_lexical_cast &e) {
            throw std::runtime_error(
                std::string("cannot parse timeout value") + value);
        }
    }
    else if (strncasecmp(name, "elapsed-time", sizeof("elapsed-time")) == 0) {
        trb_data_->check_elapsed_time_ = (strncasecmp(value, "yes", sizeof("yes")) == 0);
    }
    else {
        Block::property(name, value);
    }
}

void
ThreadedBlock::postInvoke(Context *ctx, InvokeContext *invoke_ctx) {
    
    bool show_elapsed_time = trb_data_->check_elapsed_time_ ? trb_data_->check_elapsed_time_ :
    OperationMode::instance()->checkDevelopmentVariable(ctx->request(), ThreadedBlockData::SHOW_ELAPSED_TIME);
       
    if (!show_elapsed_time || tagged()) {
        return;
    }
    
    xmlNodePtr node = xmlDocGetRootElement(invoke_ctx->resultDoc().get());
    if (NULL == node) {
        return;
    }
    
    const xmlChar* elapsed_attr = (const xmlChar*)"elapsed-time";
    std::string elapsed = boost::lexical_cast<std::string>(0.001*ctx->timer().elapsed());
    if (xmlHasProp(node, elapsed_attr) &&
        xmlUnsetProp(node, elapsed_attr) < 0) {
        log()->error("Cannot unset elapsed-time attribute");
        return;
    }
    xmlNewProp(node, elapsed_attr, (const xmlChar*)elapsed.c_str());
}

void
ThreadedBlock::startTimer(Context *ctx) {
    ctx->startTimer(invokeTimeout());
}

void
ThreadedBlock::stopTimer(Context *ctx) {
    ctx->stopTimer();
}

int
ThreadedBlock::remainedTime(Context *ctx) const {
    return ctx->timer().remained();
}

} // namespace xscript
