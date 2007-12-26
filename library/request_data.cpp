#include "settings.h"

#include <cassert>

#include "xscript/state.h"
#include "xscript/request_data.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

RequestData::RequestData() :
	request_(NULL), response_(NULL), state_(new State())
{
}

RequestData::RequestData(Request *req, Response *resp, const boost::shared_ptr<State> &state) :
	request_(req), response_(resp), state_(state)
{
	assert(NULL != request_);
	assert(NULL != response_);
	assert(NULL != state_.get());
}

RequestData::~RequestData() {
}

} // namespace xscript
