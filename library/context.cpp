#include "settings.h"

#include <cassert>
#include <stdexcept>
#include <algorithm>

#include <boost/bind.hpp>
#include <boost/current_function.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread.hpp>

#include "xscript/authorizer.h"
#include "xscript/block.h"
#include "xscript/cache_strategy.h"
#include "xscript/context.h"
#include "xscript/logger.h"
#include "xscript/policy.h"
#include "xscript/script.h"
#include "xscript/server.h"
#include "xscript/state.h"
#include "xscript/stylesheet.h"
#include "xscript/typed_map.h"
#include "xscript/vhost_data.h"
#include "xscript/xml_util.h"

#include "details/writer_impl.h"
#include "internal/extension_list.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

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

struct CommonData {
    CommonData(const boost::shared_ptr<RequestData> &request_data,
               const std::string &xslt_name) :
        request_data_(request_data), params_(new ParamsMap()),
        xslt_name_(xslt_name), no_xslt_(false), no_main_xslt_(false)
    {}
    
    CommonData(const boost::shared_ptr<RequestData> &request_data,
               const boost::shared_ptr<ParamsMap> &params,
               const boost::shared_ptr<AuthContext> &auth,
               const std::string &xslt_name) :
        request_data_(request_data), params_(params), auth_(auth),
        xslt_name_(xslt_name), no_xslt_(false), no_main_xslt_(false)
    {}
    ~CommonData() {}
    
    mutable boost::mutex mutex_;
    boost::shared_ptr<RequestData> request_data_;
    boost::shared_ptr<ParamsMap> params_;
    boost::shared_ptr<AuthContext> auth_;
    std::string xslt_name_;
    std::auto_ptr<DocumentWriter> writer_;
    bool no_xslt_;
    bool no_main_xslt_;
};

struct Context::ContextData {
    ContextData(const boost::shared_ptr<RequestData> request_data,
                const boost::shared_ptr<Script> &script) :
        stopped_(false), common_data_(new CommonData(request_data, script->xsltName())),
        script_(script), expire_delta_(-1), flags_(0)
    {
        if (!script->expireTimeDeltaUndefined()) {
            requestData()->response()->setExpireDelta(script->expireTimeDelta());
        }
    }
    
    ContextData(const boost::shared_ptr<RequestData> request_data,
                const boost::shared_ptr<Script> &script,
                const boost::shared_ptr<Context> &ctx,
                const TypedMap &local_params) :
        stopped_(false), common_data_(new CommonData(request_data, script->xsltName())),
        script_(script), parent_context_(ctx), expire_delta_(-1), flags_(0),
        local_params_(local_params)
    {
        if (!script->expireTimeDeltaUndefined()) {
            requestData()->response()->setExpireDelta(script->expireTimeDelta());
        }
    }
    
    ContextData(const boost::shared_ptr<Script> &script,
                const boost::shared_ptr<Context> &ctx,
                const boost::shared_ptr<CommonData> &common_data,
                const TypedMap &local_params) :
        stopped_(false), common_data_(common_data), script_(script), parent_context_(ctx),
        expire_delta_(-1), flags_(0), local_params_(local_params)
    {       
        if (!script->expireTimeDeltaUndefined()) {
            requestData()->response()->setExpireDelta(script->expireTimeDelta());
        }
        timer_.reset(ctx->timer().remained());
    }
    
    ~ContextData() {
        std::for_each(clear_node_list_.begin(),
                      clear_node_list_.end(),
                      boost::bind(&xmlFreeNode, _1));
    }
    
    const boost::shared_ptr<RequestData>& requestData() const {
        return common_data_->request_data_;
    }
    
    DocumentWriter* documentWriter() const {
        return common_data_->writer_.get();
    }
    
    void documentWriter(std::auto_ptr<DocumentWriter> writer) {
        common_data_->writer_ = writer;
    }
    
    const boost::shared_ptr<AuthContext>& authContext() const {
        return common_data_->auth_;
    }
    
    void authContext(const boost::shared_ptr<AuthContext> &auth) {
        common_data_->auth_ = auth;
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
    
    bool hasRuntimeError() const {
        boost::mutex::scoped_lock lock(runtime_errors_mutex_);
        return !runtime_errors_.empty();
    }
    
    bool noXsltPort() const {
        boost::mutex::scoped_lock lock(common_data_->mutex_);
        return common_data_->no_xslt_;
    }

    void noXsltPort(bool value) {
        boost::mutex::scoped_lock lock(common_data_->mutex_);
        common_data_->no_xslt_ = value;
    }

    bool noMainXsltPort() const {
        boost::mutex::scoped_lock lock(common_data_->mutex_);
        return common_data_->no_main_xslt_;
    }

    void noMainXsltPort(bool value) {
        boost::mutex::scoped_lock lock(common_data_->mutex_);
        common_data_->no_main_xslt_ = value;
    }
    
    std::string xsltName() const {
        boost::mutex::scoped_lock lock(common_data_->mutex_);
        return common_data_->xslt_name_;
    }
    
    void xsltName(const std::string &value) {
        boost::mutex::scoped_lock lock(common_data_->mutex_);
        common_data_->xslt_name_ = value;
    }
    
    bool insertParam(const std::string &key, const boost::any &value) {
        return common_data_->params_->insert(key, value);
    }

    bool findParam(const std::string &key, boost::any &value) const {
        return common_data_->params_->find(key, value);
    }
 
    volatile bool stopped_;
    boost::shared_ptr<CommonData> common_data_;
    boost::shared_ptr<Script> script_;
    boost::shared_ptr<Context> parent_context_;
    std::vector<boost::shared_ptr<InvokeContext> > results_;
    std::list<xmlNodePtr> clear_node_list_;
    boost::int32_t expire_delta_;

    boost::condition condition_;

    unsigned int flags_;

    std::map<const Block*, std::string> runtime_errors_;
    TimeoutCounter timer_;

    TypedMap local_params_;
    
    static boost::thread_specific_ptr<std::list<TimeoutCounter> > block_timers_;
    
    mutable boost::mutex attr_mutex_, results_mutex_, node_list_mutex_, runtime_errors_mutex_;
    
    static const unsigned int FLAG_FORCE_NO_THREADED = 1;
    static const unsigned int FLAG_NO_CACHE = 1 << 3;
    static const unsigned int SKIP_NEXT_BLOCKS = 1 << 4;
    static const unsigned int STOP_BLOCKS = 1 << 5;
    
};

boost::thread_specific_ptr<std::list<TimeoutCounter> > Context::ContextData::block_timers_;

Context::Context(const boost::shared_ptr<Script> &script,
                 const boost::shared_ptr<State> &state,
                 const boost::shared_ptr<Request> &request,
                 const boost::shared_ptr<Response> &response) :
    ctx_data_(NULL)
{
    assert(script.get());
    boost::shared_ptr<RequestData> request_data(
            new RequestData(request, response, state));
    ctx_data_ = new ContextData(request_data, script);
    init();
}

Context::Context(const boost::shared_ptr<Script> &script,
                 const boost::shared_ptr<Context> &ctx,
                 const TypedMap &local_params,
                 bool proxy) : ctx_data_(NULL)
{
    assert(script.get());
    assert(ctx.get());
    
    if (proxy) {
        ctx_data_ = new ContextData(script, ctx, ctx->ctx_data_->common_data_, local_params);
    }
    else {
        boost::shared_ptr<RequestData> request_data(new RequestData());
        ctx_data_ = new ContextData(request_data, script, ctx, local_params);
        ctx_data_->authContext(Authorizer::instance()->checkAuth(this));
        init();
    }
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
    ExtensionList::instance()->initContext(this);    
    const Server *server = VirtualHostData::instance()->getServer();
    if (NULL != server) {
        ctx_data_->noXsltPort(!server->needApplyPerblockStylesheet(request()));
        ctx_data_->noMainXsltPort(!server->needApplyMainStylesheet(request()));
    }
    CacheStrategy *strategy = script()->cacheStrategy();
    if (NULL != strategy) {
        strategy->initContext(this);
    }
}

boost::shared_ptr<Context>
Context::createChildContext(const boost::shared_ptr<Script> &script,
                            const boost::shared_ptr<Context> &ctx,
                            const TypedMap &local_params,
                            bool proxy) {
    return boost::shared_ptr<Context>(new Context(script, ctx, local_params, proxy));
}

const boost::shared_ptr<RequestData>&
Context::requestData() const {
    return ctx_data_->requestData();
}

Request*
Context::request() const {
    return requestData()->request();
}

Response*
Context::response() const {
    return requestData()->response();
}

State*
Context::state() const {
    return requestData()->state();
}

const boost::shared_ptr<Script>&
Context::script() const {
    return ctx_data_->script_;
}

const boost::shared_ptr<AuthContext>&
Context::authContext() const {
    return ctx_data_->authContext();
}

void
Context::wait(int millis) {

    log()->debug("%s, setting timeout: %d", BOOST_CURRENT_FUNCTION, millis);

    boost::xtime xt = delay(millis);
    boost::mutex::scoped_lock sl(ctx_data_->results_mutex_);
    bool timedout = !ctx_data_->condition_.timed_wait(sl, xt, boost::bind(&Context::resultsReady, this));
    
    bool no_cache = false;
    bool save_result = timedout || stopBlocks();
    if (save_result) {
        for (unsigned int i = 0; i < ctx_data_->results_.size(); ++i) {
            InvokeContext* result = ctx_data_->results_[i].get();
            if (NULL == result || NULL == result->resultDocPtr()) {
                if (timedout) {
                    ctx_data_->results_[i] = ctx_data_->script_->block(i)->errorResult("timed out", false);
                    no_cache = true;
                }
                else {
                    ctx_data_->results_[i] = ctx_data_->script_->block(i)->errorResult("stopped", true);
                }
            }
        }
    }
    sl.unlock();
    
    if (no_cache) {
        setNoCache();
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
Context::result(unsigned int n, boost::shared_ptr<InvokeContext> result) {
    log()->debug("%s: %d, result of %u block: %p", BOOST_CURRENT_FUNCTION,
                 static_cast<int>(stopped()), n, result->resultDocPtr());
    
    if (result.get()) {
        boost::shared_ptr<Context> local_ctx = result->getLocalContext();
        result->setLocalContext(boost::shared_ptr<Context>()); // circle reference removed
        if (local_ctx.get()) {
            local_ctx->stop();
        }
    }
    
    boost::mutex::scoped_lock sl(ctx_data_->results_mutex_);
    if (!stopped() && ctx_data_->results_.size() != 0) {
        if (NULL == ctx_data_->results_[n].get()) {
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
    if (stopBlocks()) {
        return true;
    }
    for (std::vector<boost::shared_ptr<InvokeContext> >::const_iterator it = ctx_data_->results_.begin(),
            end = ctx_data_->results_.end(); it != end; ++it) {
        if (NULL == it->get()) {
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

boost::shared_ptr<InvokeContext>
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
    return ctx_data_->xsltName();
}

void
Context::xsltName(const std::string &value) {
    boost::mutex::scoped_lock sl(ctx_data_->attr_mutex_);
    if (value.empty()) {
        ctx_data_->xsltName(value);
    }
    else {
        ctx_data_->xsltName(script()->fullName(value));
    }
}

void
Context::authContext(const boost::shared_ptr<AuthContext> &auth) {
    ctx_data_->authContext(auth);
}

DocumentWriter*
Context::documentWriter() {
    if (NULL == ctx_data_->documentWriter()) {
        ctx_data_->documentWriter(std::auto_ptr<DocumentWriter>(
            new XmlWriter(Policy::getOutputEncoding(request()))));
    }

    return ctx_data_->documentWriter();
}

void
Context::createDocumentWriter(const boost::shared_ptr<Stylesheet> &sh) {
    if (sh->outputMethod() == "xml" && !sh->haveOutputInfo()) {
        ctx_data_->documentWriter(std::auto_ptr<DocumentWriter>(new XmlWriter(sh->outputEncoding())));
        log()->debug("xml writer created");
    }
    else {
        ctx_data_->documentWriter(std::auto_ptr<DocumentWriter>(new HtmlWriter(sh)));
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
    return ctx_data_->noXsltPort();
}

bool
Context::noMainXsltPort() const {
    return ctx_data_->noMainXsltPort();
}

bool
Context::noCache() const {
    return ctx_data_->flag(ContextData::FLAG_NO_CACHE);
}

void
Context::setNoCache() {
    Context *local_ctx = this;
    while(local_ctx) {
        local_ctx->ctx_data_->flag(ContextData::FLAG_NO_CACHE, true);
        local_ctx = local_ctx->parentContext();
    }
}

bool
Context::skipNextBlocks() const {
    return ctx_data_->flag(ContextData::SKIP_NEXT_BLOCKS);
}

void
Context::skipNextBlocks(bool value) {
    ctx_data_->flag(ContextData::SKIP_NEXT_BLOCKS, value);
}

bool
Context::stopBlocks() const {
    return ctx_data_->flag(ContextData::STOP_BLOCKS);
}

void
Context::stopBlocks(bool value) {
    ctx_data_->flag(ContextData::STOP_BLOCKS, value);
}

void
Context::setExpireDelta(boost::int32_t delta) {
    ctx_data_->expire_delta_ = delta;
}

boost::int32_t
Context::expireDelta() const {
    return ctx_data_->expire_delta_;
}

bool
Context::expireDeltaUndefined() const {
    return ctx_data_->expire_delta_ == -1;
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

Context*
Context::originalContext() const {
    const Context* ctx = this;
    while(ctx->isProxy()) {
        ctx = ctx->parentContext();
        if (NULL == ctx) {
            throw std::logic_error("NULL original context");
        }
    }
    return const_cast<Context*>(ctx); 
}

bool
Context::isRoot() const {
    return parentContext() == NULL;
}

bool
Context::isProxy() const {
    if (isRoot()) {
        return false;
    }
    return request() == parentContext()->request();
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

const TimeoutCounter&
Context::timer() const {
    std::list<TimeoutCounter> *timers = ContextData::block_timers_.get();
    if (NULL == timers || timers->empty()) {
        return ctx_data_->timer_;
    }
    return timers->back();
}

void
Context::resetTimer() {
    std::list<TimeoutCounter> *timers = ContextData::block_timers_.get();
    if (NULL != timers) {
        timers->clear();
    }
}

void
Context::startTimer(int timeout) {
    std::list<TimeoutCounter> *timers = ContextData::block_timers_.get();
    if (NULL == timers) {
        ContextData::block_timers_.reset(new std::list<TimeoutCounter>);
        timers = ContextData::block_timers_.get();
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
    std::list<TimeoutCounter> *timers = ContextData::block_timers_.get();
    if (NULL == timers || timers->empty()) {
        throw std::runtime_error("Cannot stop timer that is not exist");
    }
    
    timers->pop_back();
}

bool
Context::insertParam(const std::string &key, const boost::any &value) {
    return ctx_data_->insertParam(key, value);
}

bool
Context::findParam(const std::string &key, boost::any &value) const {
    return ctx_data_->findParam(key, value);
}

bool
Context::hasLocalParam(const std::string &name) const {
    return ctx_data_->local_params_.has(name);
}

bool
Context::localParamIs(const std::string &name) const {
    return ctx_data_->local_params_.is(name);
}

const TypedValue&
Context::getLocalParam(const std::string &name) const {
    return ctx_data_->local_params_.find(name);
}

bool
Context::getLocalParam(const std::string &name, TypedValue &result) const {
    return ctx_data_->local_params_.find(name, result);
}

std::string
Context::getLocalParam(const std::string &name, const std::string &default_value) const {
    return ctx_data_->local_params_.asString(name, default_value);
}

void
Context::localParams(std::map<std::string, TypedValue> &params) const {
    ctx_data_->local_params_.values(params);
}

void
Context::stop() {
    if (!isProxy()) {
        ExtensionList::instance()->stopContext(this);
    }
       
    if (noCache() || ctx_data_->hasRuntimeError()) {
        rootContext()->setNoCache();
    }
    
    ctx_data_->stopped_ = true;
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
    if (ctx_.get()) {
        ctx_->stop();
    }
}

void
ContextStopper::reset() {
    return ctx_.reset();
}

} // namespace xscript
