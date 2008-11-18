#include "settings.h"

#include <iostream>
#include <set>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

#include <xscript/logger.h>
#include <xscript/policy.h>

#include "standalone_request.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

StandaloneRequest::StandaloneRequest() :
    isSecure_(false), port_(80), path_("/"), method_("GET") {
}

StandaloneRequest::~StandaloneRequest() {
}

unsigned short
StandaloneRequest::getServerPort() const {
    return port_;
}

const std::string&
StandaloneRequest::getServerAddr() const {
    return host_;
}

const std::string&
StandaloneRequest::getPathInfo() const {
    return path_;
}

const std::string&
StandaloneRequest::getPathTranslated() const {
    return path_;
}

const std::string&
StandaloneRequest::getScriptName() const {
    return script_name_;
}

const std::string&
StandaloneRequest::getScriptFilename() const {
    return script_file_name_;
}

const std::string&
StandaloneRequest::getDocumentRoot() const {
    return root_directory_;
}

const std::string&
StandaloneRequest::getQueryString() const {
    return query_;
}

const std::string&
StandaloneRequest::getRequestMethod() const {
    return method_;
}

bool
StandaloneRequest::isSecure() const {
    return isSecure_;
}

std::streamsize
StandaloneRequest::write(const char *buf, std::streamsize size) {
    std::cout << std::endl;
    std::cout.write(buf, size);
    return size;
}

void
StandaloneRequest::sendError(unsigned short status, const std::string& message) {
    std::cerr << status << std::endl << message << std::endl;
}

void
StandaloneRequest::attach(const std::string& url, const std::string& doc_root) {

    if (url.empty()) {
        throw std::runtime_error("Cannot process empty url");
    }

    root_directory_ = doc_root;
    std::string processed_url = Policy::instance()->getPathByScheme(this, url);
    std::string root_directory_ = Policy::instance()->getRootByScheme(this, url);

    size_t base_pos = 0;
    size_t pos = processed_url.find("://");
    if (pos == std::string::npos) {
        if (processed_url[0] != '/') {
            int max_path_size = 8192;
            char dirname[max_path_size];
            char *ret = getcwd(dirname, max_path_size);
            if (ret == NULL) {
                throw std::runtime_error("working directory path is too long");
            }

            std::string full_path = std::string(dirname) + "/" + processed_url;

            namespace fs = boost::filesystem;
            fs::path path(full_path);
            path.normalize();
            processed_url = path.native_file_string();
        }
    }
    else {
        std::string protocol = processed_url.substr(0, pos);

        if (protocol == "http") {
            isSecure_ = false;
        }
        else if (protocol == "https"){
            isSecure_ = true;
        }
        else {
            throw std::runtime_error("Protocol is not supported: " + protocol);
        }

        pos += 3;
        base_pos = processed_url.find('/', pos);
        std::string host = processed_url.substr(pos, base_pos - pos);

        size_t port_pos = host.find(':');
        if (port_pos == std::string::npos) {
            port_ = 80;
            host_ = host;
        }
        else {
            host_ = host.substr(0, port_pos);
            std::string port = host.substr(port_pos + 1);
            try {
                port_ = boost::lexical_cast<unsigned short>(port);
            }
            catch(const boost::bad_lexical_cast &e) {
                throw std::runtime_error("Cannot parse port: " + port);
            }
        }
    }

    if (base_pos != std::string::npos) {
        std::string path = processed_url.substr(base_pos);
        size_t size = root_directory_.size();
        if (size <= path.size() &&
            strncmp(root_directory_.c_str(), path.c_str(), size) == 0) {
            path = path.substr(size);
        }

        size_t query_pos = path.find('?');
        if (query_pos != std::string::npos) {
            query_ = path.substr(query_pos + 1);
        }

        size_t slesh_pos = path.rfind('/', query_pos) + 1;
        script_name_ = path.substr(slesh_pos, query_pos - slesh_pos);
        path_ = path.substr(0, slesh_pos);
    }

    script_file_name_ = root_directory_ + path_ + script_name_;
    namespace fs = boost::filesystem;
    fs::path path(script_file_name_);
    path.normalize();
    script_file_name_ = path.native_file_string();

    typedef boost::char_separator<char> Separator;
    typedef boost::tokenizer<Separator> Tokenizer;

    Tokenizer tok(query_, Separator("&"));
    for (Tokenizer::iterator it = tok.begin(), it_end = tok.end(); it != it_end; ++it) {
        std::string::size_type pos = it->find('=');
        if (std::string::npos != pos && pos < it->size() - 1) {
            setArg(it->substr(0, pos), it->substr(pos + 1));
        }
    }
}

} // namespace xscript
