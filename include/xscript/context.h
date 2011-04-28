#ifndef _XSCRIPT_CONTEXT_H_
#define _XSCRIPT_CONTEXT_H_

#include <map>
#include <string>
#include <stdexcept>
#include <vector>

#include <boost/any.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/xtime.hpp>

#include <libxml/tree.h>

#include "xscript/invoke_context.h"
#include "xscript/request.h"
#include "xscript/response.h"
#include "xscript/request_data.h"
#include "xscript/util.h"

namespace xscript {

class AuthContext;
class Block;
class DocumentWriter;
class Script;
class State;
class Stylesheet;
class TypedMap;
class TypedValue;

/**
 * Class for storing Context during single page processing.
 *
 * Stores incoming Request, outgoing Response, State and classical
 * key-value pairs for any required data.
 */
class Context : private boost::noncopyable {
public:
    Context(const boost::shared_ptr<Script> &script,
            const boost::shared_ptr<State> &state,
            const boost::shared_ptr<Request> &request,
            const boost::shared_ptr<Response> &response);
    virtual ~Context();

    enum {
	PROXY_NONE = 0,
	PROXY_REQUEST = 1,
	PROXY_OTHERS = 2, // response, state, lua ...
	PROXY_ALL = PROXY_REQUEST | PROXY_OTHERS
    };

    static boost::shared_ptr<Context> createChildContext(
            const boost::shared_ptr<Script> &script,
            const boost::shared_ptr<Context> &ctx,
            const boost::shared_ptr<InvokeContext> &invoke_ctx,
            const boost::shared_ptr<TypedMap> &local_params,
            unsigned int proxy_flags);
    
    void wait(const boost::xtime &until);
    void expect(unsigned int count);
    void result(unsigned int n, boost::shared_ptr<InvokeContext> result);
    void addNode(xmlNodePtr node);
    void addDoc(XmlDocSharedHelper doc);
    void addDoc(XmlDocHelper doc);

    bool resultsReady() const;
    static boost::xtime delay(int millis);
    boost::shared_ptr<InvokeContext> result(unsigned int n) const;

    const boost::shared_ptr<RequestData>& requestData() const;
    Request* request() const;
    Response* response() const;
    State* state() const;
    const boost::shared_ptr<Script>& script() const;

    const boost::shared_ptr<Context>& parentContext() const;
    Context* rootContext() const;
    Context* originalContext() const;
    Context* originalStateContext() const;

    bool isRoot() const;
    bool isProxy() const;

    InvokeContext* invokeContext() const;

    bool hasXslt() const;
    std::string xsltName() const;
    void xsltName(const std::string &value);
    
    const boost::shared_ptr<AuthContext>& authContext() const;

    void authContext(const boost::shared_ptr<AuthContext> &auth);

    DocumentWriter* documentWriter();
    void documentWriter(DocumentWriter *writer);

    void createDocumentWriter(const boost::shared_ptr<Stylesheet> &sh);

    template<typename T> T param(const std::string &name) const;
    template<typename T> void param(const std::string &name, const T &t);

    // Get or create param
    typedef boost::shared_ptr<boost::mutex> MutexPtr;

    template<typename T> T param(
        const std::string &name, const boost::function<T ()> &creator);
    
    template<typename T> T param(
        const std::string &name, const boost::function<T ()> &creator, boost::mutex &mutex);

    bool stopped() const;

    bool noXsltPort() const;
    bool noMainXsltPort() const;
    bool noCache() const;
    void setNoCache();
    bool skipNextBlocks() const;
    void skipNextBlocks(bool value);
    bool stopBlocks() const;
    void stopBlocks(bool value);
    std::string getRuntimeError(const Block *block) const;
    void assignRuntimeError(const Block *block, const std::string &error_message);

    const TimeoutCounter& timer() const;
    void startTimer(int timeout);
    void stopTimer();
    static void resetTimer();
    
    bool xsltChanged(const Script *script) const;

    void setExpireDelta(boost::int32_t delta);
    boost::int32_t expireDelta() const;
    bool expireDeltaUndefined() const;
    
    bool hasLocalParam(const std::string &name) const;
    bool localParamIs(const std::string &name) const;
    const TypedValue& getLocalParam(const std::string &name) const;
    const std::string& getLocalParam(const std::string &name, const std::string &default_value) const;
    const std::map<std::string, TypedValue>& localParams() const;
    const boost::shared_ptr<TypedMap>& localParamsMap() const;

    static const std::string CREATE_DOCUMENT_WRITER_METHOD;

    friend class ContextStopper;

private:
    Context(const boost::shared_ptr<Script> &script,
            const boost::shared_ptr<Context> &ctx,
            InvokeContext *invoke_ctx,
            const boost::shared_ptr<TypedMap> &local_params,
            unsigned int proxy_flags);
    void init();
    bool insertParam(const std::string &key, const boost::any &value);
    bool findParam(const std::string &key, boost::any &value) const;
    void stop();

private:
    struct ContextData;
    std::auto_ptr<ContextData> ctx_data_;
};

class ContextStopper {
public:
    ContextStopper(boost::shared_ptr<Context> ctx);
    ~ContextStopper();
    
    void reset();
private:
    boost::shared_ptr<Context> ctx_;
};

template<typename T> inline T
Context::param(const std::string &name) const {
    boost::any value;
    if (findParam(name, value)) {
        return boost::any_cast<T>(value);
    }
    else {
        throw std::invalid_argument(std::string("nonexistent param: ").append(name));
    }
}

template<typename T> inline void
Context::param(const std::string &name, const T &t) {
    if (!insertParam(name, boost::any(t))) {
        throw std::invalid_argument(std::string("duplicate param: ").append(name));
    }
}

template<typename T> inline T
Context::param(const std::string &name, const boost::function<T ()> &creator) {
    boost::any value;
    if (findParam(name, value)) {
        return boost::any_cast<T>(value);
    }
    else {
        T t = creator();
        insertParam(name, boost::any(t));
        return t;
    }
}

template<typename T> inline T
Context::param(const std::string &name, const boost::function<T ()> &creator, boost::mutex &mutex) {
    boost::mutex::scoped_lock lock(mutex);
    return param(name, creator);
}

} // namespace xscript

#endif // _XSCRIPT_CONTEXT_H_
