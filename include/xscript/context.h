#ifndef _XSCRIPT_CONTEXT_H_
#define _XSCRIPT_CONTEXT_H_

#include <map>
#include <vector>
#include <string>
#include <stdexcept>
#include <boost/any.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <libxml/tree.h>

#include "xscript/request_data.h"
#include "xscript/util.h"
#include "xscript/invoke_result.h"

namespace xscript {

class Block;
class State;
class Script;
class Request;
class Response;
class Stylesheet;
class AuthContext;
class DocumentWriter;

/**
 * Class for storing Context during single page processing.
 *
 * Stores incoming Request, outgoing Response, State and classical
 * key-value pairs for any required data.
 */
class Context : private boost::noncopyable {
public:
    Context(const boost::shared_ptr<Script> &script,
            const boost::shared_ptr<RequestData> &data);
    virtual ~Context();

    boost::shared_ptr<Context> createChildContext(const boost::shared_ptr<Script> &script);
    
    void wait(int millis);
    void expect(unsigned int count);
    void result(unsigned int n, InvokeResult result);
    void addNode(xmlNodePtr node);

    bool resultsReady() const;
    boost::xtime delay(int millis) const;
    InvokeResult result(unsigned int n) const;

    inline const boost::shared_ptr<RequestData>& requestData() const {
        return request_data_;
    }

    inline Request* request() const {
        return request_data_->request();
    }

    inline Response* response() const {
        return request_data_->response();
    }

    inline State* state() const {
        return request_data_->state();
    }

    inline const boost::shared_ptr<Script>& script() const {
        return script_;
    }

    Context* parentContext() const;
    Context* rootContext() const;
    bool isRoot() const;

    std::string xsltName() const;
    void xsltName(const std::string &value);

    inline const boost::shared_ptr<AuthContext>& authContext() const {
        return auth_;
    }

    void authContext(const boost::shared_ptr<AuthContext> &auth);

    DocumentWriter* documentWriter();

    void createDocumentWriter(const boost::shared_ptr<Stylesheet> &sh);

    template<typename T> T param(const std::string &name) const;
    template<typename T> void param(const std::string &name, const T &t);

    // Get or create param
    template<typename T> T param(const std::string &name, const boost::function<T ()> &creator);

    bool stopped() const;

    bool forceNoThreaded() const;
    void forceNoThreaded(bool value);
    bool noXsltPort() const;
    void noXsltPort(bool value);
    std::string getRuntimeError(const Block *block) const;
    void assignRuntimeError(const Block *block, const std::string &error_message);

    const TimeoutCounter& timer() const;
    void startTimer(int timeout);
    void stopTimer();

    friend class ContextStopper;

private:
    Context(const boost::shared_ptr<Script> &script, Context *ctx);
    void init();
    void stop();
    
    inline void flag(unsigned int type, bool value) {
        flags_ = value ? (flags_ | type) : (flags_ &= ~type);
    }

private:
    
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

private:
    volatile bool stopped_;
    boost::shared_ptr<RequestData> request_data_;
    Context* parent_context_;
    std::string xslt_name_;
    std::vector<InvokeResult> results_;
    std::list<xmlNodePtr> clear_node_list_;

    boost::condition condition_;
    boost::shared_ptr<Script> script_;

    boost::shared_ptr<AuthContext> auth_;
    std::auto_ptr<DocumentWriter> writer_;

    unsigned int flags_;

    std::map<const Block*, std::string> runtime_errors_;
    boost::thread_specific_ptr<std::list<TimeoutCounter> > block_timers_;
    TimeoutCounter timer_;

    boost::shared_ptr<ParamsMap> params_;
    
    mutable boost::mutex attr_mutex_, results_mutex_, node_list_mutex_, runtime_errors_mutex_;

    static const unsigned int FLAG_FORCE_NO_THREADED = 1;
    static const unsigned int FLAG_NO_XSLT_PORT = 1 << 1;
    
    static TimeoutCounter default_timer_;
};

class ContextStopper {
public:
    ContextStopper(boost::shared_ptr<Context> ctx);
    ~ContextStopper();
private:
    boost::shared_ptr<Context> ctx_;
};

template<typename T> inline T
Context::param(const std::string &name) const {
    boost::any value;
    if (params_->find(name, value)) {
        return boost::any_cast<T>(value);
    }
    else {
        throw std::invalid_argument(std::string("nonexistent param: ").append(name));
    }
}

template<typename T> inline void
Context::param(const std::string &name, const T &t) {
    if (!params_->insert(name, boost::any(t))) {
        throw std::invalid_argument(std::string("duplicate param: ").append(name));
    }
}

template<typename T> inline T
Context::param(const std::string &name, const boost::function<T ()> &creator) {
    boost::any value;
    if (params_->find(name, value)) {
        return boost::any_cast<T>(value);
    }
    else {
        T t = creator();
        params_->insert(name, boost::any(t));
        return t;
    }
}

} // namespace xscript

#endif // _XSCRIPT_CONTEXT_H_
