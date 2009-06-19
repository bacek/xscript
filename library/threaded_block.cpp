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

const std::string ThreadedBlock::SHOW_ELAPSED_TIME("SHOW_ELAPSED_TIME");

ThreadedBlock::ThreadedBlock(const Extension *ext, Xml *owner, xmlNodePtr node) :
        Block(ext, owner, node), threaded_(false), timeout_(0), check_elapsed_time_(false) {
}

ThreadedBlock::~ThreadedBlock() {
}

int
ThreadedBlock::timeout() const {
// FIXME: Change hard-coded 5000 to configurable defaults.
    return timeout_ > 0 ? timeout_ : 5000;
}

int
ThreadedBlock::invokeTimeout() const {
    return timeout();
}

bool
ThreadedBlock::threaded() const {
    return threaded_;
}

void
ThreadedBlock::threaded(bool value) {
    threaded_ = value;
}

void
ThreadedBlock::property(const char *name, const char *value) {
    if (strncasecmp(name, "threaded", sizeof("threaded")) == 0) {
        threaded_ = (strncasecmp(value, "yes", sizeof("yes")) == 0);
    }
    else if (strncasecmp(name, "timeout", sizeof("timeout")) == 0) {
        try {
            timeout_ = boost::lexical_cast<int>(value);
        }
        catch(const boost::bad_lexical_cast &e) {
            throw std::runtime_error(
                std::string("cannot parse timeout value") + value);
        }
    }
    else if (strncasecmp(name, "elapsed-time", sizeof("elapsed-time")) == 0) {
        check_elapsed_time_ = (strncasecmp(value, "yes", sizeof("yes")) == 0);
    }
    else {
        Block::property(name, value);
    }
}

void
ThreadedBlock::postInvoke(Context *ctx, const XmlDocHelper &doc) {
    
    bool show_elapsed_time = check_elapsed_time_ ? check_elapsed_time_ :
        OperationMode::instance()->checkDevelopmentVariable(ctx->request(), SHOW_ELAPSED_TIME);
       
    if (!show_elapsed_time || tagged()) {
        return;
    }
    xmlNodePtr node = xmlDocGetRootElement(doc.get());
    if (NULL == node) {
        return;
    }
    std::string elapsed = boost::lexical_cast<std::string>(0.001*ctx->timer().elapsed());
    xmlNewProp(node, (const xmlChar*)"elapsed-time", (const xmlChar*)elapsed.c_str());
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
