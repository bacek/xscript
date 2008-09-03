#ifndef _XSCRIPT_REQUEST_DATA_H_
#define _XSCRIPT_REQUEST_DATA_H_

#include <boost/shared_ptr.hpp>

#include "xscript/request_response.h"

namespace xscript {

class State;

class RequestData {
public:
    RequestData();
    RequestData(const boost::shared_ptr<RequestResponse> &req,
                const boost::shared_ptr<State> &state);
    virtual ~RequestData();

    inline State* state() const {
        return state_.get();
    }

    inline Request* request() const {
        return request_response_.get();
    }

    inline Response* response() const {
        return request_response_.get();
    }

private:
    boost::shared_ptr<RequestResponse> request_response_;
    boost::shared_ptr<State> state_;
};

} // namespace xscript

#endif // _XSCRIPT_REQUEST_DATA_H_
