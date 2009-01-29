#include "settings.h"
#include <cstring>
#include <boost/lexical_cast.hpp>

#include "xscript/context.h"
#include "xscript/threaded_block.h"
#include "xscript/script.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

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
        timeout_ = boost::lexical_cast<int>(value);
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
    if (!check_elapsed_time_ || tagged()) {
        return;
    }
    xmlNodePtr node = xmlDocGetRootElement(doc.get());
    if (NULL == node) {
        return;
    }
    std::string elapsed = boost::lexical_cast<std::string>(0.001*ctx->blockTimer(this).elapsed());
    xmlNewProp(node, (const xmlChar*)"elapsed-time", (const xmlChar*)elapsed.c_str());
}

void
ThreadedBlock::startTimer(Context *ctx) {
    ctx->startBlockTimer(this, std::min(ctx->timer().remained(), invokeTimeout()));
}

} // namespace xscript
