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

namespace xscript
{

class State;
class Script;
class Request;
class Response;
class Stylesheet;
class AuthContext;
class RequestData;
class DocumentWriter;

/**
 * Class for storing Context during single page processing.
 *
 * Stores incoming Request, outgoing Response, State and classical
 * key-value pairs for any required data. 
 */
class Context : private boost::noncopyable
{
public:
	Context(const boost::shared_ptr<Script> &script, const RequestData &data);
	virtual ~Context();

	void wait(int millis);
	void expect(unsigned int count);
	void result(unsigned int n, xmlDocPtr doc);
	void addNode(xmlNodePtr node);
	
	bool resultsReady() const;
	boost::xtime delay(int millis) const;
	xmlDocPtr result(unsigned int n) const;
	
	inline Request* request() const {
		return request_;
	}
	
	inline Response* response() const {
		return response_;
	}
	
	inline const boost::shared_ptr<State>& state() const {
		return state_;
	}
	
	inline const boost::shared_ptr<Script>& script() const {
		return script_;
	}

	void forceNoThreaded(bool flag);
	bool forceNoThreaded() const;

	void parentContext(Context* context);
	Context* parentContext() const;

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
    template<typename T> T param(const std::string &name, const boost::function<T ()>& creator);

	friend class ContextStopper;
private:
	volatile bool stopped_;
	bool force_no_threaded_;
	Request *request_;
	Response *response_;
	Context* parent_context_;
	std::string xslt_name_;
	std::vector<xmlDocPtr> results_;
	std::list<xmlNodePtr> clear_node_list_;
	
	boost::condition condition_;
	boost::shared_ptr<State> state_;
	boost::shared_ptr<Script> script_;

	boost::shared_ptr<AuthContext> auth_;
	std::auto_ptr<DocumentWriter> writer_;
	
	std::map<std::string, boost::any> params_;
	mutable boost::mutex params_mutex_, results_mutex_, node_list_mutex_;
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
	boost::mutex::scoped_lock sl(params_mutex_);
	std::map<std::string, boost::any>::const_iterator i = params_.find(name);
	if (params_.end() != i) {
		return boost::any_cast<T>(i->second);
	}
	else {
		throw std::invalid_argument(std::string("nonexistent param: ").append(name)); 
	}
}

template<typename T> inline void
Context::param(const std::string &name, const T &t) {
	std::pair<std::string, boost::any> p(name, boost::any(t));
	boost::mutex::scoped_lock sl(params_mutex_);
	std::map<std::string, boost::any>::iterator i = params_.find(name);
	if (params_.end() == i) {
		params_.insert(p);
	}
	else {
		throw std::invalid_argument(std::string("duplicate param: ").append(name)); 
	}
}

template<typename T> inline T
Context::param(const std::string &name, const boost::function<T ()>& creator) {
	boost::mutex::scoped_lock sl(params_mutex_);
	std::map<std::string, boost::any>::const_iterator i = params_.find(name);
	if (params_.end() != i) {
		return boost::any_cast<T>(i->second);
	}
	else {
        T t = creator();
	    std::pair<std::string, boost::any> p(name, boost::any(t));
		params_.insert(p);
        return t;
	}
}

} // namespace xscript

#endif // _XSCRIPT_CONTEXT_H_
