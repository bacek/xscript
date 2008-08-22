#ifndef _XSCRIPT_VHOST_DATA_H_
#define _XSCRIPT_VHOST_DATA_H_

#include <string>
#include <vector>
#include <boost/utility.hpp>
#include <boost/thread/tss.hpp>
#include <libxml/tree.h>

#include <xscript/component.h>
#include <xscript/request.h>
#include <xscript/server.h>

namespace xscript {

class VirtualHostData : public Component<VirtualHostData> {
public:
    VirtualHostData();
    virtual ~VirtualHostData();

    void set(const Request* request);
    const Server* getServer() const;

    virtual bool hasVariable(const Request* request, const std::string& var) const;
    virtual std::string getVariable(const Request* request, const std::string& var) const;
    virtual bool checkVariable(Request* request, const std::string& var) const;
    virtual std::string getKey(const Request* request, const std::string& name) const;
    virtual std::string getOutputEncoding(const Request* request) const;

    friend class Server;
protected:
    void setServer(const Server* server);
    const Request* get() const;

private:
    class RequestProvider {
    public:
        RequestProvider(const Request* request) : request_(request) {}
        const Request* get() const {
            return request_;
        }
    private:
        const Request* request_;
    };

    boost::thread_specific_ptr<RequestProvider> request_provider_;
    const Server* server_;
};

} // namespace xscript

#endif // _XSCRIPT_VHOST_DATA_H_
