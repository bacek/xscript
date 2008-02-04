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

#include <fcgio.h>
#include <fcgiapp.h>

#include <libxml/tree.h>
#include <libxml/xmlsave.h>
#include <libxml/encoding.h>
#include <libxslt/xsltutils.h>

#include "server.h"
#include "request.h"
#include "xscript/util.h"
#include "xscript/state.h"
#include "xscript/config.h"
#include "xscript/logger.h"
#include "xscript/script.h"
#include "xscript/writer.h"
#include "xscript/context.h"
#include "xscript/authorizer.h"
#include "xscript/request_data.h"
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
	socket_(-1), config_(config), inbuf_size_(0), outbuf_size_(0), alternate_port_(0)
{
	if (0 != FCGX_Init()) {
		throw std::runtime_error("can not init fastcgi library");
	}
	config->startup();
}

Server::~Server() {
}

void
Server::run() {
	
	std::string socket = config_->as<std::string>("/xscript/endpoint/socket");
	unsigned short backlog = config_->as<unsigned short>("/xscript/endpoint/backlog");
	
	pid(config_->as<std::string>("/xscript/pidfile"));
	
	inbuf_size_ = config_->as<unsigned int>("/xscript/input-buffer", 4096);
	outbuf_size_ = config_->as<unsigned int>("/xscript/output-buffer", 4096);
	
	alternate_port_ = config_->as<unsigned short>("/xscript/alternate-port", 8080);
	
	if (!socket.empty()) {
		socket_ = FCGX_OpenSocket(socket.c_str(), backlog);
		chmod(socket.c_str(), 0666);
	}
	else {
		std::stringstream stream;
		stream << ':' << config_->as<unsigned short>("/xscript/endpoint/port");
		socket_ = FCGX_OpenSocket(stream.str().c_str(), backlog);
	}
	
	if (-1 == socket_) {
		throw std::runtime_error("can not open fastcgi socket");
	}
	
	boost::function<void()> f = boost::bind(&Server::handle, this);
	unsigned short pool_size = config_->as<unsigned short>("/xscript/fastcgi-workers");
	for (unsigned short i = 0; i < pool_size; ++i) {
		create_thread(f);
	}
	join_all();
}

void
Server::handle() {
	
	FCGX_Request req;
	std::vector<char> inv(inbuf_size_), outv(outbuf_size_);
	
	if (0 != FCGX_InitRequest(&req, socket_, 0)) {
		log()->error("can not init fastcgi request");
		return;
	}

	XmlUtils::registerReporters();

	ServerRequest request;
	
	while (true) {
		if (-1 != FCGX_Accept_r(&req)) {
			try {
				fcgi_streambuf inbuf(req.in, &inv[0], inv.size());
				fcgi_streambuf outbuf(req.out, &outv[0], outv.size());
			
				std::istream is(&inbuf);
				std::ostream os(&outbuf);
			
				request.attach(&is, &os, req.envp);
				handleRequest(&request);
				
				os << std::flush;
				
				request.reset();
				FCGX_Finish_r(&req);
			}
			catch (const std::exception &e) {
				request.reset();
				FCGX_Finish_r(&req);
				log()->error("caught exception while handling request: %s", e.what());
			}
		}
		else {
			log()->error("failed to accept fastcgi request");
		}
	}
	FCGX_Free(&req, 1);
}

void
Server::pid(const std::string &file) {
	try {
		std::ofstream f(file.c_str());
		f.exceptions(std::ios::badbit);
		f << static_cast<int>(getpid());
	}
	catch (std::ios::failure &e) {
		std::cerr << "can not open file " << file << std::endl;
		throw;
	}
}

void
Server::handleRequest(ServerRequest *request) {
	
	RequestData data(request, request, boost::shared_ptr<State>(new State()));
	VirtualHostData::instance()->set(request);
	xmlOutputBufferPtr buf = NULL;
	try {
		
		log()->info("requested file: %s", request->getScriptFilename().c_str());
		std::pair<std::string, bool> name = findScript(request->getScriptFilename());

		if (!name.second) {
			CheckingPolicy::instance()->sendError(request, 404, request->getScriptFilename() + " not found");
			return;
		}
		
		boost::shared_ptr<Script> script = Script::create(name.first);
		if (!script->allowMethod(request->getRequestMethod())) {
			CheckingPolicy::instance()->sendError(request, 405, request->getRequestMethod() + " not allowed");
			return;
		}

		Authorizer *authorizer = Authorizer::instance();
		boost::shared_ptr<Context> ctx(new Context(script, data));
		ContextStopper ctx_stopper(ctx);
		std::auto_ptr<AuthContext> auth = authorizer->checkAuth(ctx);
		assert(NULL != auth.get());
		
		if (!auth->authorized()) {
			authorizer->redirectToAuth(ctx, auth.get());
			request->sendHeaders();
			return;
		}
		ctx->authContext(auth);
		
		XmlDocHelper doc = script->invoke(ctx);
		XmlUtils::throwUnless(NULL != doc.get());
		
		if (script->forceStylesheet() && needApplyStylesheet(request)) {
			log()->info("applying stylesheet to %s", script->name().c_str());
			script->applyStylesheet(ctx.get(), doc);
		}
		else {
			script->removeUnusedNodes(doc);
		}

		DocumentWriter* writer = ctx->documentWriter();
		assert(NULL != writer);
			
		writer->addHeaders(request);
		request->sendHeaders();
		
		if (!request->suppressBody()) {
			xmlCharEncodingHandlerPtr encoder = NULL;
			const std::string &encoding = writer->outputEncoding();
			if (!encoding.empty()) {
				encoder = xmlFindCharEncodingHandler(encoding.c_str());
			}
			buf = xmlOutputBufferCreateIO(&writeFunc, &closeFunc, ctx.get(), encoder);
			XmlUtils::throwUnless(NULL != buf);
			writer->write(request, doc, buf);
		}
	}
	catch (const std::exception &e) {
		log()->error("%s: exception caught: %s\n", BOOST_CURRENT_FUNCTION, e.what());
		xmlOutputBufferClose(buf);
		CheckingPolicy::instance()->sendError(request, 500, e.what());
	}
}

bool
Server::needApplyStylesheet(ServerRequest *request) const {
	return (request->getServerPort() != alternate_port_);
}

std::pair<std::string, bool>
Server::findScript(const std::string &name) const {
	
	namespace fs = boost::filesystem;
	fs::path path(name);
	bool path_exists = fs::exists(path);

	if (path_exists && fs::is_directory(path)) {
		path = path / "index.html";
		path_exists = fs::exists(path);
	}

	return std::make_pair(path.native_file_string(), path_exists);
}

bool
Server::fileExists(const std::string &name) const {

	namespace fs = boost::filesystem;
	fs::path path(name);
	return fs::exists(path) && !fs::is_directory(path);
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
