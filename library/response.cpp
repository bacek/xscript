#include "settings.h"
#include "xscript/request.h"
#include "xscript/response.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

Response::Response() 
{
}

Response::~Response() {
}

void
Response::redirectBack(const Request *req) {
	redirectToPath(req->getHeader("Referer"));
}

void
Response::redirectToPath(const std::string &path) {
	setStatus(302);
	setHeader("Location", path);
}

void
Response::setContentType(const std::string &type) {
	setHeader("Content-type", type);
}

void
Response::setContentEncoding(const std::string &encoding) {
	setHeader("Content-encoding", encoding);
}

} // namespace xscript
