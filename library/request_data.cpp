#include "settings.h"

#include <cassert>

#include "xscript/request.h"
#include "xscript/request_data.h"
#include "xscript/response.h"
#include "xscript/state.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

RequestData::RequestData() : request_(boost::shared_ptr<Request>(new Request())),
                             response_(boost::shared_ptr<Response>(new Response())),
                             state_(new State()) {
}

RequestData::RequestData(const boost::shared_ptr<Request> &req,
                         const boost::shared_ptr<Response> &resp,
                         const boost::shared_ptr<State> &state) :
    request_(req), response_(resp), state_(state) {
    
    assert(NULL != request_.get());
    assert(NULL != response_.get());
    assert(NULL != state_.get());
}

RequestData::~RequestData() {
}

} // namespace xscript
