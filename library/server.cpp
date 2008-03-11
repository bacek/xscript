#include "settings.h"

#include <cerrno>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/current_function.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <unistd.h>
#include <sys/stat.h>

#include <libxml/tree.h>
#include <libxml/xmlsave.h>
#include <libxml/encoding.h>
#include <libxslt/xsltutils.h>

#include "xscript/server.h"
#include "xscript/util.h"
#include "xscript/state.h"
#include "xscript/config.h"
#include "xscript/logger.h"
#include "xscript/script.h"
#include "xscript/writer.h"
#include "xscript/context.h"
#include "xscript/authorizer.h"
#include "xscript/request_data.h"
#include "xscript/response.h"
#include "xscript/vhost_data.h"
#include "xscript/checking_policy.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

extern "C" int closeFunc(void *ctx);
extern "C" int writeFunc(void *ctx, const char *data, int len);

Server::Server(Config *config) : 
	config_(config)
{
	config_->startup();
}

Server::~Server() {
}

void
Server::handleRequest(RequestData *request_data) {

	VirtualHostData::instance()->set(request_data->request());
	XmlUtils::resetReporter();
	xmlOutputBufferPtr buf = NULL;
	try {
		log()->info("requested file: %s", request_data->request()->getScriptFilename().c_str());
		std::pair<std::string, bool> name = findScript(request_data->request()->getScriptFilename());

		if (!name.second) {
			CheckingPolicy::instance()->sendError(request_data->response(), 404,
				request_data->request()->getScriptFilename() + " not found");
			return;
		}

		boost::shared_ptr<Script> script = Script::create(name.first);
		if (!script->allowMethod(request_data->request()->getRequestMethod())) {
			CheckingPolicy::instance()->sendError(request_data->response(), 405,
				request_data->request()->getRequestMethod() + " not allowed");
			return;
		}

		Authorizer *authorizer = Authorizer::instance();
		boost::shared_ptr<Context> ctx(new Context(script, *request_data));
		ContextStopper ctx_stopper(ctx);
		std::auto_ptr<AuthContext> auth = authorizer->checkAuth(ctx);
		assert(NULL != auth.get());

		if (!auth->authorized()) {
			authorizer->redirectToAuth(ctx, auth.get());
			request_data->response()->sendHeaders();
			return;
		}
		ctx->authContext(auth);

		XmlDocHelper doc = script->invoke(ctx);
		XmlUtils::throwUnless(NULL != doc.get());

		if (script->forceStylesheet() && needApplyStylesheet(request_data->request())) {
			log()->info("applying stylesheet to %s", script->name().c_str());
			script->applyStylesheet(ctx.get(), doc);
		}
		else {
			script->removeUnusedNodes(doc);
		}

		DocumentWriter* writer = ctx->documentWriter();
		assert(NULL != writer);

		writer->addHeaders(request_data->response());
		request_data->response()->sendHeaders();

		if (!request_data->request()->suppressBody()) {
			xmlCharEncodingHandlerPtr encoder = NULL;
			const std::string &encoding = writer->outputEncoding();
			if (!encoding.empty()) {
				encoder = xmlFindCharEncodingHandler(encoding.c_str());
			}
			buf = xmlOutputBufferCreateIO(&writeFunc, &closeFunc, ctx.get(), encoder);
			XmlUtils::throwUnless(NULL != buf);
			writer->write(request_data->response(), doc, buf);
		}
	}
	catch (const std::exception &e) {
		log()->error("%s: exception caught: %s", BOOST_CURRENT_FUNCTION, e.what());
		xmlOutputBufferClose(buf);
		CheckingPolicy::instance()->sendError(request_data->response(), 500, e.what());
	}
}

bool
Server::needApplyStylesheet(Request *request) const {
	return true;
}

std::pair<std::string, bool>
Server::findScript(const std::string &name) {

	namespace fs = boost::filesystem;
	fs::path path(name);
	bool path_exists = fs::exists(path);

	if (path_exists && fs::is_directory(path)) {
		path = path / "index.html";
		path_exists = fs::exists(path);
	}

	return std::make_pair(path.native_file_string(), path_exists);
}

extern "C" int
closeFunc(void *ctx) {
	return 0;
}

extern "C" int
writeFunc(void *ctx, const char *data, int len) {
	if (0 == len) {
		return 0;
	}
	Context *context = static_cast<Context*>(ctx);
	try {
		return context->response()->write(data, len);
	}
	catch (const std::exception &e) {
		log()->error("caught exception while writing result: %s %s", 
		context->request()->getScriptFilename().c_str(), e.what());
	}
	return -1;
}

} // namespace xscript
