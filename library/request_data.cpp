#include "settings.h"

#include <cassert>

#include "xscript/state.h"
#include "xscript/request_data.h"

#include "internal/default_request_response.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

RequestData::RequestData() :
    request_response_(boost::shared_ptr<RequestResponse>()), state_(new State()) {
}

RequestData::RequestData(const boost::shared_ptr<RequestResponse> &req,
                         const boost::shared_ptr<State> &state) :
    request_response_(req), state_(state) {
    assert(NULL != request_response_.get());
    assert(NULL != state_.get());
}

RequestData::~RequestData() {
}

} // namespace xscript
