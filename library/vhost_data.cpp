#include "settings.h"

#include "xscript/request.h"
#include "xscript/string_utils.h"
#include "xscript/util.h"
#include "xscript/vhost_data.h"

#include <boost/lexical_cast.hpp>
#include <boost/thread/tss.hpp>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class VirtualHostData::HostData {
public:
    HostData() : server_(NULL) {}
    ~HostData() {}
    
    const Request* get() const {
        RequestProvider* provider = request_provider_.get();
        if (NULL == provider) {
            return NULL;
        }

        return provider->get();
    }
    
    class RequestProvider {
    public:
        RequestProvider(const Request *request) : request_(request) {}
        const Request* get() const {
            return request_;
        }
    private:
        const Request* request_;
    };

    const Server* server_;
    boost::thread_specific_ptr<RequestProvider> request_provider_;
};

VirtualHostData::VirtualHostData() : data_(new HostData()) {
}

VirtualHostData::~VirtualHostData() {
    delete data_;
}

void
VirtualHostData::set(const Request *request) {
    data_->request_provider_.reset(new HostData::RequestProvider(request));
}

void
VirtualHostData::setServer(const Server *server) {
    data_->server_ = server;
}

const Server*
VirtualHostData::getServer() const {
    return data_->server_;
}

bool
VirtualHostData::hasVariable(const Request *request, const std::string &var) const {
    if (NULL == request) {
        request = data_->get();
        if (NULL == request) {
            return false;
        }
    }

    return request->hasVariable(var);
}

std::string
VirtualHostData::getVariable(const Request *request, const std::string &var) const {
    if (NULL == request) {
        request = data_->get();
        if (NULL == request) {
            return StringUtils::EMPTY_STRING;
        }
    }

    return request->getVariable(var);
}

bool
VirtualHostData::checkVariable(const Request *request, const std::string &var) const {

    if (hasVariable(request, var)) {
        std::string value = VirtualHostData::instance()->getVariable(request, var);
        if (value.empty()) {
            return false;
        }
        try {
            if (strncasecmp("yes", value.c_str(), sizeof("yes")) == 0 ||
                strncasecmp("true", value.c_str(), sizeof("true")) == 0 ||
                boost::lexical_cast<bool>(value) == 1) {
                return true;
            }
        }
        catch(boost::bad_lexical_cast &) {
            std::stringstream stream;
            stream << "Cannot cast to bool environment variable " << var
                   << ". Value: " << value;
            throw std::runtime_error(stream.str());
        }
    }

    return false;
}

std::string
VirtualHostData::getDocumentRoot(const Request *request) const {
    if (NULL == request) {
        request = data_->get();
        if (NULL == request) {
            return StringUtils::EMPTY_STRING;
        }
    }

    std::string root = request->getDocumentRoot();
    while(!root.empty() && *root.rbegin() == '/') {
        root.erase(root.length() - 1, 1);
    }
    return root;
}

static ComponentRegisterer<VirtualHostData> reg;

} // namespace xscript
