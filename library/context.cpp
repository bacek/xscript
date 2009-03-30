#include "settings.h"

#include <cassert>
#include <stdexcept>
#include <algorithm>

#include <boost/bind.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/current_function.hpp>

#include "xscript/authorizer.h"
#include "xscript/block.h"
#include "xscript/context.h"
#include "xscript/logger.h"
#include "xscript/policy.h"
#include "xscript/request_data.h"
#include "xscript/script.h"
#include "xscript/server.h"
#include "xscript/state.h"
#include "xscript/stylesheet.h"
#include "xscript/vhost_data.h"
#include "xscript/xml_util.h"

#include "details/writer_impl.h"
#include "internal/extension_list.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

Context::Context(const boost::shared_ptr<Script> &script,
                 const boost::shared_ptr<RequestData> &data) :
        stopped_(false), request_data_(data), parent_context_(NULL), xslt_name_(script->xsltName()),
        script_(script), writer_(), flags_(0),
        params_(new ParamsMap()) {

    assert(script_.get());
    init();
}

Context::Context(const boost::shared_ptr<Script> &script, Context *ctx) :
        stopped_(false), parent_context_(ctx), xslt_name_(script->xsltName()),
        script_(script), writer_(), flags_(0) {

    assert(script_.get());
    assert(ctx);

    request_data_ = ctx->request_data_;
    params_ = ctx->params_;
    auth_ = ctx->auth_;

    timer_.reset(ctx->timer().remained());
    
    init();
}

Context::~Context() {
    if (XmlUtils::hasXMLError()) {
        std::string postfix = "Script: " + script()->name();
        XmlUtils::printXMLError(postfix);
    }
    ExtensionList::instance()->destroyContext(this);
    std::for_each(clear_node_list_.begin(), clear_node_list_.end(), boost::bind(&xmlFreeNode, _1));
}

void
Context::init() {
    if (isRoot()) {
        ExtensionList::instance()->initContext(this);
    }
    const Server *server = VirtualHostData::instance()->getServer();
    if (NULL != server) {
        noXsltPort(!server->needApplyPerblockStylesheet(request()));
    }
}

boost::shared_ptr<Context>
Context::createChildContext(const boost::shared_ptr<Script> &script) {
    return boost::shared_ptr<Context>(new Context(script, this));
}

void
Context::wait(int millis) {

    log()->debug("%s, setting timeout: %d", BOOST_CURRENT_FUNCTION, millis);

    boost::xtime xt = delay(millis);
    boost::mutex::scoped_lock sl(results_mutex_);
    bool timedout = !condition_.timed_wait(sl, xt, boost::bind(&Context::resultsReady, this));

    if (timedout) {
        for (std::vector<xmlDocPtr>::size_type i = 0; i < results_.size(); ++i) {
            if (NULL == results_[i].doc.get()) {
                results_[i] = script_->block(i)->errorResult("timed out");
            }
        }
    }
}

void
Context::expect(unsigned int count) {

    boost::mutex::scoped_lock sl(results_mutex_);
    if (stopped()) {
        throw std::logic_error("context already stopped");
    }
    if (results_.size() == 0) {
        results_.resize(count);
    }
    else {
        throw std::logic_error("context already started");
    }
}

void
Context::result(unsigned int n, InvokeResult result) {

    log()->debug("%s: %d, result of %u block: %p", BOOST_CURRENT_FUNCTION,
                 static_cast<int>(stopped()), n, result.doc.get());

    boost::mutex::scoped_lock sl(results_mutex_);
    if (!stopped() && results_.size() != 0) {
        if (NULL == results_[n].doc.get()) {
            results_[n] = result;
            condition_.notify_all();
        }
    }
    else {
        log()->debug("%s, error in block %u: context not started or timed out", BOOST_CURRENT_FUNCTION, n);
    }
}

bool
Context::resultsReady() const {
    log()->debug("%s", BOOST_CURRENT_FUNCTION);
    for (std::vector<InvokeResult>::const_iterator i = results_.begin(), end = results_.end(); i != end; ++i) {
        if (NULL == i->doc.get()) {
            return false;
        }
    }
    return true;
}

void
Context::addNode(xmlNodePtr node) {

    boost::mutex::scoped_lock sl(node_list_mutex_);
    clear_node_list_.push_back(node);
}

boost::xtime
Context::delay(int millis) const {

    boost::xtime xt;
    boost::xtime_get(&xt, boost::TIME_UTC);
    boost::uint64_t usec = (xt.sec * 1000000) + (xt.nsec / 1000) + (millis * 1000);

    xt.sec = usec / 1000000;
    xt.nsec = (usec % 1000000) * 1000;

    return xt;
}

InvokeResult
Context::result(unsigned int n) const {

    boost::mutex::scoped_lock sl(results_mutex_);
    if (results_.size() > 0) {
        return results_[n];
    }
    else {
        throw std::logic_error("not started");
    }
}

std::string
Context::xsltName() const {
    boost::mutex::scoped_lock sl(attr_mutex_);
    return xslt_name_;
}

void
Context::xsltName(const std::string &value) {
    boost::mutex::scoped_lock sl(attr_mutex_);
    if (value.empty()) {
        xslt_name_.erase();
    }
    else {
        xslt_name_ = script_->fullName(value);
    }
}

void
Context::authContext(const boost::shared_ptr<AuthContext> &auth) {
    auth_ = auth;
}

DocumentWriter*
Context::documentWriter() {
    if (NULL == writer_.get()) {
        writer_ = std::auto_ptr<DocumentWriter>(
            new XmlWriter(Policy::instance()->getOutputEncoding(request())));
    }

    return writer_.get();
}

void
Context::createDocumentWriter(const boost::shared_ptr<Stylesheet> &sh) {
    if (sh->outputMethod() == "xml" && !sh->haveOutputInfo()) {
        writer_ = std::auto_ptr<DocumentWriter>(new XmlWriter(sh->outputEncoding()));
        log()->debug("xml writer created");
    }
    else {
        writer_ = std::auto_ptr<DocumentWriter>(new HtmlWriter(sh));
        log()->debug("html writer created");
    }
}

bool
Context::forceNoThreaded() const {
    boost::mutex::scoped_lock lock(attr_mutex_);
    return flags_ & FLAG_FORCE_NO_THREADED;
}

void
Context::forceNoThreaded(bool value) {
    boost::mutex::scoped_lock lock(attr_mutex_);
    flag(FLAG_FORCE_NO_THREADED, value);
}

bool
Context::noXsltPort() const {
    boost::mutex::scoped_lock lock(attr_mutex_);
    return flags_ & FLAG_NO_XSLT_PORT;
}

void
Context::noXsltPort(bool value) {
    boost::mutex::scoped_lock lock(attr_mutex_);
    flag(FLAG_NO_XSLT_PORT, value);
}

std::string
Context::getRuntimeError(const Block *block) const {
    boost::mutex::scoped_lock lock(runtime_errors_mutex_);
    std::map<const Block*, std::string>::const_iterator it = runtime_errors_.find(block);
    if (runtime_errors_.end() == it) {
        return StringUtils::EMPTY_STRING;
    }
    return it->second;
}

void
Context::assignRuntimeError(const Block *block, const std::string &error_message) {
    boost::mutex::scoped_lock lock(runtime_errors_mutex_);
    runtime_errors_[block].assign(error_message);
}

Context*
Context::parentContext() const {
    return parent_context_;
}

Context*
Context::rootContext() const {
    const Context* ctx = this;
    while (NULL != ctx->parent_context_) {
        ctx = ctx->parent_context_;
    }
    return const_cast<Context*>(ctx);
}

bool
Context::isRoot() const {
    return parent_context_ == NULL;
}

bool
Context::stopped() const {
    if (parent_context_) {
        return stopped_ || parent_context_->stopped();
    }

    return stopped_;
}

void
Context::stop() {
    stopped_ = true;
}

const TimeoutCounter&
Context::timer() const {
    std::list<TimeoutCounter> *timers = block_timers_.get();
    if (NULL == timers || timers->empty()) {
        return timer_;
    }
    return timers->back();
}

void
Context::startTimer(int timeout) {
    std::list<TimeoutCounter> *timers = block_timers_.get();
    if (NULL == timers) {
        block_timers_.reset(new std::list<TimeoutCounter>);
        timers = block_timers_.get();
    }
    if (timers->empty()) {
        timers->push_back(TimeoutCounter(std::min(timeout, timer_.remained())));
    }
    else {
        timers->push_back(TimeoutCounter(std::min(timeout, timers->back().remained())));
    }
}

void
Context::stopTimer() {
    std::list<TimeoutCounter> *timers = block_timers_.get();
    if (NULL == timers) {
        throw std::runtime_error("Cannot stop timer that is not exist");
    }
    
    timers->pop_back();
}

bool
Context::ParamsMap::insert(const std::string &name, const boost::any &value) {
    boost::mutex::scoped_lock sl(mutex_);
    iterator it = params_.find(name);
    if (params_.end() != it) {
        return false;
    }
    params_.insert(std::make_pair(name, value));
    return true;
}

bool
Context::ParamsMap::find(const std::string &name, boost::any &value) const {
    boost::mutex::scoped_lock sl(mutex_);
    iterator it = params_.find(name);
    if (params_.end() == it) {
        return false;
    }
    value = it->second;
    return true;
}

ContextStopper::ContextStopper(boost::shared_ptr<Context> ctx) : ctx_(ctx) {
}

ContextStopper::~ContextStopper() {
    if (ctx_->isRoot()) {
        ExtensionList::instance()->stopContext(ctx_.get());
    }
    ctx_->stop();
}

} // namespace xscript
