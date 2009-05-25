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

boost::thread_specific_ptr<std::list<TimeoutCounter> > Context::block_timers_;

class ParamsMap {
public:
    typedef std::map<std::string, boost::any> MapType;
    typedef MapType::const_iterator iterator;
    
    bool insert(const std::string &key, const boost::any &value);
    bool find(const std::string &key, boost::any &value) const;
private:
    mutable boost::mutex mutex_;
    MapType params_;
};

struct Context::ContextData {
    ContextData(const boost::shared_ptr<Script> &script,
                const boost::shared_ptr<RequestData> &data) :
        stopped_(false), script_(script), request_data_(data),
        xslt_name_(script->xsltName()), flags_(0), page_random_(-1),
        params_(boost::shared_ptr<ParamsMap>(new ParamsMap())) 
    {}
    
    ContextData(const boost::shared_ptr<Script> &script,
                const boost::shared_ptr<Context> &ctx,
                const boost::shared_ptr<ParamsMap> &params) :
        stopped_(false), script_(script), request_data_(ctx->requestData()), parent_context_(ctx),
        xslt_name_(script->xsltName()), auth_(ctx->authContext()), flags_(0), page_random_(-1),
        params_(params)
    {
        timer_.reset(ctx->timer().remained());
    }
    
    ~ContextData() {
        std::for_each(clear_node_list_.begin(),
                      clear_node_list_.end(),
                      boost::bind(&xmlFreeNode, _1));
    }
    
    void flag(unsigned int type, bool value) {
        boost::mutex::scoped_lock lock(attr_mutex_);
        flags_ = value ? (flags_ | type) : (flags_ &= ~type);
    }
    
    bool flag(unsigned int type) const {
        boost::mutex::scoped_lock lock(attr_mutex_);
        return flags_ & type;
    }
    
    void addNode(xmlNodePtr node) {
        boost::mutex::scoped_lock sl(node_list_mutex_);
        clear_node_list_.push_back(node);
    }

    std::string getRuntimeError(const Block *block) const {
        boost::mutex::scoped_lock lock(runtime_errors_mutex_);
        std::map<const Block*, std::string>::const_iterator it = runtime_errors_.find(block);
        if (runtime_errors_.end() == it) {
            return StringUtils::EMPTY_STRING;
        }
        return it->second;
    }

    void assignRuntimeError(const Block *block, const std::string &error_message) {
        boost::mutex::scoped_lock lock(runtime_errors_mutex_);
        runtime_errors_[block].assign(error_message);
    }
    
    boost::int32_t pageRandom() {
        if (page_random_ < 0) {
            boost::int32_t random_max = script_->pageRandomMax();
            page_random_ = random_max > 0 ? random() % (random_max + 1) : 0;
        }
        return page_random_;
    }
    
    volatile bool stopped_;
    boost::shared_ptr<Script> script_;
    boost::shared_ptr<RequestData> request_data_;
    boost::shared_ptr<Context> parent_context_;
    std::string xslt_name_;
    std::vector<InvokeResult> results_;
    std::list<xmlNodePtr> clear_node_list_;

    boost::condition condition_;

    boost::shared_ptr<AuthContext> auth_;
    std::auto_ptr<DocumentWriter> writer_;
    
    std::string key_;

    unsigned int flags_;
    
    boost::int32_t page_random_;

    std::map<const Block*, std::string> runtime_errors_;
    TimeoutCounter timer_;

    boost::shared_ptr<ParamsMap> params_;
    
    mutable boost::mutex attr_mutex_, results_mutex_, node_list_mutex_, runtime_errors_mutex_;
    
    static const unsigned int FLAG_FORCE_NO_THREADED = 1;
    static const unsigned int FLAG_NO_XSLT_PORT = 1 << 1;
    static const unsigned int FLAG_NO_CACHE = 1 << 2;
};

Context::Context(const boost::shared_ptr<Script> &script,
                 const boost::shared_ptr<RequestData> &data) : ctx_data_(NULL)
{
    assert(script.get());
    ctx_data_ = new ContextData(script, data);
    init();
}

Context::Context(const boost::shared_ptr<Script> &script,
                 const boost::shared_ptr<Context> &ctx) : ctx_data_(NULL)
{
    assert(script.get());
    assert(ctx.get());
    ctx_data_ = new ContextData(script, ctx, ctx->ctx_data_->params_);
    init();
}

Context::~Context() {
    if (XmlUtils::hasXMLError()) {
        std::string postfix = "Script: " + script()->name();
        XmlUtils::printXMLError(postfix);
    }
    ExtensionList::instance()->destroyContext(this);
    delete ctx_data_;
}

void
Context::init() {
    if (isRoot()) {
        ExtensionList::instance()->initContext(this);
        appendKey(boost::lexical_cast<std::string>(pageRandom()));
    }
    const Server *server = VirtualHostData::instance()->getServer();
    if (NULL != server) {
        noXsltPort(!server->needApplyPerblockStylesheet(request()));
    }
}

boost::shared_ptr<Context>
Context::createChildContext(const boost::shared_ptr<Script> &script, const boost::shared_ptr<Context> &ctx) {
    return boost::shared_ptr<Context>(new Context(script, ctx));
}

const boost::shared_ptr<RequestData>&
Context::requestData() const {
    return ctx_data_->request_data_;
}

Request*
Context::request() const {
    return ctx_data_->request_data_->request();
}

Response*
Context::response() const {
    return ctx_data_->request_data_->response();
}

State*
Context::state() const {
    return ctx_data_->request_data_->state();
}

const boost::shared_ptr<Script>&
Context::script() const {
    return ctx_data_->script_;
}

const boost::shared_ptr<AuthContext>&
Context::authContext() const {
    return ctx_data_->auth_;
}

void
Context::wait(int millis) {

    log()->debug("%s, setting timeout: %d", BOOST_CURRENT_FUNCTION, millis);

    boost::xtime xt = delay(millis);
    boost::mutex::scoped_lock sl(ctx_data_->results_mutex_);
    bool timedout = !ctx_data_->condition_.timed_wait(sl, xt, boost::bind(&Context::resultsReady, this));

    if (timedout) {
        for (std::vector<xmlDocPtr>::size_type i = 0; i < ctx_data_->results_.size(); ++i) {
            if (NULL == ctx_data_->results_[i].doc.get()) {
                ctx_data_->results_[i] = ctx_data_->script_->block(i)->errorResult("timed out");
            }
        }
    }
}

void
Context::expect(unsigned int count) {

    boost::mutex::scoped_lock sl(ctx_data_->results_mutex_);
    if (stopped()) {
        throw std::logic_error("context already stopped");
    }
    if (ctx_data_->results_.size() == 0) {
        ctx_data_->results_.resize(count);
    }
    else {
        throw std::logic_error("context already started");
    }
}

void
Context::result(unsigned int n, InvokeResult result) {

    log()->debug("%s: %d, result of %u block: %p", BOOST_CURRENT_FUNCTION,
                 static_cast<int>(stopped()), n, result.doc.get());

    boost::mutex::scoped_lock sl(ctx_data_->results_mutex_);
    if (!stopped() && ctx_data_->results_.size() != 0) {
        if (NULL == ctx_data_->results_[n].doc.get()) {
            ctx_data_->results_[n] = result;
            ctx_data_->condition_.notify_all();
        }
    }
    else {
        log()->debug("%s, error in block %u: context not started or timed out", BOOST_CURRENT_FUNCTION, n);
    }
}

bool
Context::resultsReady() const {
    log()->debug("%s", BOOST_CURRENT_FUNCTION);
    for (std::vector<InvokeResult>::const_iterator i = ctx_data_->results_.begin(),
            end = ctx_data_->results_.end(); i != end; ++i) {
        if (NULL == i->doc.get()) {
            return false;
        }
    }
    return true;
}

void
Context::addNode(xmlNodePtr node) {
    ctx_data_->addNode(node);
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

    boost::mutex::scoped_lock sl(ctx_data_->results_mutex_);
    if (ctx_data_->results_.size() > 0) {
        return ctx_data_->results_[n];
    }
    else {
        throw std::logic_error("not started");
    }
}

std::string
Context::xsltName() const {
    boost::mutex::scoped_lock sl(ctx_data_->attr_mutex_);
    return ctx_data_->xslt_name_;
}

void
Context::xsltName(const std::string &value) {
    boost::mutex::scoped_lock sl(ctx_data_->attr_mutex_);
    if (value.empty()) {
        ctx_data_->xslt_name_.erase();
    }
    else {
        ctx_data_->xslt_name_ = ctx_data_->script_->fullName(value);
    }
}

void
Context::authContext(const boost::shared_ptr<AuthContext> &auth) {
    ctx_data_->auth_ = auth;
}

DocumentWriter*
Context::documentWriter() {
    if (NULL == ctx_data_->writer_.get()) {
        ctx_data_->writer_ = std::auto_ptr<DocumentWriter>(
            new XmlWriter(Policy::instance()->getOutputEncoding(request())));
    }

    return ctx_data_->writer_.get();
}

void
Context::createDocumentWriter(const boost::shared_ptr<Stylesheet> &sh) {
    if (sh->outputMethod() == "xml" && !sh->haveOutputInfo()) {
        ctx_data_->writer_ = std::auto_ptr<DocumentWriter>(new XmlWriter(sh->outputEncoding()));
        log()->debug("xml writer created");
    }
    else {
        ctx_data_->writer_ = std::auto_ptr<DocumentWriter>(new HtmlWriter(sh));
        log()->debug("html writer created");
    }
}

bool
Context::forceNoThreaded() const {
    return ctx_data_->flag(ContextData::FLAG_FORCE_NO_THREADED);
}

void
Context::forceNoThreaded(bool value) {
    ctx_data_->flag(ContextData::FLAG_FORCE_NO_THREADED, value);
}

bool
Context::noXsltPort() const {
    return ctx_data_->flag(ContextData::FLAG_NO_XSLT_PORT);
}

void
Context::noXsltPort(bool value) {
    ctx_data_->flag(ContextData::FLAG_NO_XSLT_PORT, value);
}

bool
Context::noCache() const {
    return ctx_data_->flag(ContextData::FLAG_NO_CACHE);
}

void
Context::setNoCache() {
    ctx_data_->flag(ContextData::FLAG_NO_CACHE, true);
}

std::string
Context::getRuntimeError(const Block *block) const {
    return ctx_data_->getRuntimeError(block);
}

void
Context::assignRuntimeError(const Block *block, const std::string &error_message) {
    ctx_data_->assignRuntimeError(block, error_message);
}

Context*
Context::parentContext() const {
    return ctx_data_->parent_context_.get();
}

Context*
Context::rootContext() const {
    const Context* ctx = this;
    while (NULL != ctx->parentContext()) {
        ctx = ctx->parentContext();
    }
    return const_cast<Context*>(ctx);
}

bool
Context::isRoot() const {
    return parentContext() == NULL;
}

bool
Context::stopped() const {
    if (parentContext()) {
        return ctx_data_->stopped_ || parentContext()->stopped();
    }

    return ctx_data_->stopped_;
}

bool
Context::xsltChanged(const Script *script) const {
    return script->xsltName() != xsltName();
}

const std::string&
Context::key() const {
    return ctx_data_->key_;
}

void
Context::key(const std::string &key) {
    ctx_data_->key_ = key;
}

void
Context::appendKey(const std::string &value) {
    std::string key_str = key();
    if (!key_str.empty()) {
        key_str.push_back('|');
    }
    key_str.append(value);
    key(key_str);
}

boost::int32_t
Context::pageRandom() const {
    return ctx_data_->pageRandom();
}

const TimeoutCounter&
Context::timer() const {
    std::list<TimeoutCounter> *timers = block_timers_.get();
    if (NULL == timers || timers->empty()) {
        return ctx_data_->timer_;
    }
    return timers->back();
}

void
Context::resetTimer() {
    std::list<TimeoutCounter> *timers = block_timers_.get();
    if (NULL != timers) {
        timers->clear();
    }
}

void
Context::startTimer(int timeout) {
    std::list<TimeoutCounter> *timers = block_timers_.get();
    if (NULL == timers) {
        block_timers_.reset(new std::list<TimeoutCounter>);
        timers = block_timers_.get();
    }
    if (timers->empty()) {
        timers->push_back(TimeoutCounter(std::min(timeout, ctx_data_->timer_.remained())));
    }
    else {
        timers->push_back(TimeoutCounter(std::min(timeout, timers->back().remained())));
    }
}

void
Context::stopTimer() {
    std::list<TimeoutCounter> *timers = block_timers_.get();
    if (NULL == timers || timers->empty()) {
        throw std::runtime_error("Cannot stop timer that is not exist");
    }
    
    timers->pop_back();
}

bool
Context::insertParam(const std::string &key, const boost::any &value) {
    return ctx_data_->params_->insert(key, value);
}

bool
Context::findParam(const std::string &key, boost::any &value) const {
    return ctx_data_->params_->find(key, value);
}

bool
ParamsMap::insert(const std::string &name, const boost::any &value) {
    boost::mutex::scoped_lock sl(mutex_);
    iterator it = params_.find(name);
    if (params_.end() != it) {
        return false;
    }
    params_.insert(std::make_pair(name, value));
    return true;
}

bool
ParamsMap::find(const std::string &name, boost::any &value) const {
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
    else if (ctx_->noCache() || !ctx_->ctx_data_->runtime_errors_.empty()) {
        ctx_->rootContext()->setNoCache();
    }
    
    ctx_->ctx_data_->stopped_ = true;
}

} // namespace xscript
