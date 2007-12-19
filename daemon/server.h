#ifndef _XSCRIPT_SERVER_H_
#define _XSCRIPT_SERVER_H_

#include <string>
#include <boost/thread.hpp>

namespace xscript
{

class Config;
class ServerRequest;

class Server : private boost::thread_group
{
public:
	Server(Config *config);
	virtual ~Server();
	
	void run();

protected:
	void handle();
	void pid(const std::string &file);
	void handleRequest(ServerRequest *request);
	
	bool needApplyStylesheet(ServerRequest *request) const;
	std::pair<std::string, bool> findScript(const std::string &name) const;
	bool fileExists(const std::string &name) const;
	
private:
	Server(const Server &);
	Server& operator = (const Server &);

private:
	int socket_;
	Config *config_;
	int inbuf_size_, outbuf_size_;
	unsigned short alternate_port_;
};

} // namespace xscript

#endif // _XSCRIPT_SERVER_H_
