#ifndef _XSCRIPT_DAEMON_SERVER_H_
#define _XSCRIPT_DAEMON_SERVER_H_

#include <string>
#include <boost/thread.hpp>

#include "xscript/server.h"

namespace xscript
{

class Config;
class Request;

class FCGIServer : public Server, private boost::thread_group
{
public:
	FCGIServer(Config *config);
	virtual ~FCGIServer();
	
	void run();

protected:
	void handle();
	void pid(const std::string &file);
	
	bool needApplyStylesheet(Request *request) const;
	bool fileExists(const std::string &name) const;

private:
	int socket_;
	int inbuf_size_, outbuf_size_;
	unsigned short alternate_port_;
};

} // namespace xscript

#endif // _XSCRIPT_DAEMON_SERVER_H_
