#ifndef _XSCRIPT_VHOST_DATA_H_
#define _XSCRIPT_VHOST_DATA_H_

#include <string>

#include <xscript/component.h>
#include <xscript/server.h>

namespace xscript {

class Request;

class VirtualHostData : public Component<VirtualHostData> {
public:
    VirtualHostData();
    virtual ~VirtualHostData();

    void set(const Request *request);
    const Server* getServer() const;

    bool hasVariable(const Request *request, const std::string &var) const;
    std::string getVariable(const Request *request, const std::string &var) const;
    bool checkVariable(const Request *request, const std::string &var) const;

    std::string getDocumentRoot(const Request *request) const;

protected:
    friend class Server;
    void setServer(const Server *server);

private:
    class HostData;
    HostData *data_;
};

} // namespace xscript

#endif // _XSCRIPT_VHOST_DATA_H_
