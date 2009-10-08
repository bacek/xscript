#ifndef _XSCRIPT_REQUEST_DATA_H_
#define _XSCRIPT_REQUEST_DATA_H_

#include <boost/shared_ptr.hpp>

namespace xscript {

class Request;
class Response;
class State;

class RequestData {
public:
    RequestData();
    RequestData(const boost::shared_ptr<Request> &req,
                const boost::shared_ptr<Response> &resp,
                const boost::shared_ptr<State> &state);
    virtual ~RequestData();

    inline State* state() const {
        return state_.get();
    }

    inline Request* request() const {
        return request_.get();
    }

    inline Response* response() const {
        return response_.get();
    }

private:
    boost::shared_ptr<Request> request_;
    boost::shared_ptr<Response> response_;
    boost::shared_ptr<State> state_;
};

} // namespace xscript

#endif // _XSCRIPT_REQUEST_DATA_H_
