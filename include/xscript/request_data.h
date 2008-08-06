#ifndef _XSCRIPT_REQUEST_DATA_H_
#define _XSCRIPT_REQUEST_DATA_H_

#include <boost/shared_ptr.hpp>

namespace xscript {

class State;
class Request;
class Response;

class RequestData {
public:
    RequestData();
    RequestData(Request *req, Response *resp, const boost::shared_ptr<State> &state);
    virtual ~RequestData();

    inline Request* request() const {
        return request_;
    }

    inline Response* response() const {
        return response_;
    }

    inline const boost::shared_ptr<State>& state() const {
        return state_;
    }

private:
    Request *request_;
    Response *response_;
    boost::shared_ptr<State> state_;
};

} // namespace xscript

#endif // _XSCRIPT_REQUEST_DATA_H_
