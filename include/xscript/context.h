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

#include "xscript/invoke_result.h"
#include "xscript/request_data.h"
#include "xscript/util.h"

namespace xscript {

class AuthContext;
class Block;
class DocumentWriter;
class Request;
class Response;
class Script;
class State;
class Stylesheet;

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

    static boost::shared_ptr<Context> createChildContext(
            const boost::shared_ptr<Script> &script, const boost::shared_ptr<Context> &ctx);
    
    void wait(int millis);
    void expect(unsigned int count);
    void result(unsigned int n, InvokeResult result);
    void addNode(xmlNodePtr node);

    bool resultsReady() const;
    boost::xtime delay(int millis) const;
    InvokeResult result(unsigned int n) const;

    const boost::shared_ptr<RequestData>& requestData() const;
    Request* request() const;
    Response* response() const;
    State* state() const;
    const boost::shared_ptr<Script>& script() const;

    Context* parentContext() const;
    Context* rootContext() const;
    bool isRoot() const;

    std::string xsltName() const;
    void xsltName(const std::string &value);

    unsigned int expireTimeDelta() const;
    void expireTimeDelta(unsigned int value);
    
    const boost::shared_ptr<AuthContext>& authContext() const;

    void authContext(const boost::shared_ptr<AuthContext> &auth);

    DocumentWriter* documentWriter();

    void createDocumentWriter(const boost::shared_ptr<Stylesheet> &sh);

    template<typename T> T param(const std::string &name) const;
    template<typename T> void param(const std::string &name, const T &t);

    // Get or create param
    template<typename T> T param(
        const std::string &name, const boost::function<T ()> &creator, boost::mutex &mutex);

    bool stopped() const;

    bool forceNoThreaded() const;
    void forceNoThreaded(bool value);
    bool noXsltPort() const;
    void noXsltPort(bool value);
    bool noMainXsltPort() const;
    void noMainXsltPort(bool value);
    bool noCache() const;
    void setNoCache();
    bool suppressBody() const;
    void suppressBody(bool value);
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
    
    const std::string& key() const;
    void key(const std::string &key);
    void appendKey(const std::string &value);
    
    boost::int32_t pageRandom() const;

    friend class ContextStopper;

private:
    Context(const boost::shared_ptr<Script> &script, const boost::shared_ptr<Context> &ctx);
    void init();
    bool insertParam(const std::string &key, const boost::any &value);
    bool findParam(const std::string &key, boost::any &value) const;

private:
    struct ContextData;
    ContextData *ctx_data_;
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
Context::param(const std::string &name, const boost::function<T ()> &creator, boost::mutex &mutex) {
    boost::any value;
    boost::mutex::scoped_lock lock(mutex);
    if (findParam(name, value)) {
        return boost::any_cast<T>(value);
    }
    else {
        T t = creator();
        insertParam(name, boost::any(t));
        return t;
    }
}

} // namespace xscript

#endif // _XSCRIPT_CONTEXT_H_
