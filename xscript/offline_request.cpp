#include "settings.h"

#include <iostream>
#include <set>

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

#include <xscript/logger.h>
#include <xscript/policy.h>
#include <xscript/util.h>
#include <internal/algorithm.h>

#include "offline_request.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

OfflineRequest::OfflineRequest() :
    data_stream_(NULL), error_stream_(NULL) {
}

OfflineRequest::~OfflineRequest() {
}

void
OfflineRequest::writeBuffer(const char *buf, std::streamsize size) {
    data_stream_->write(buf, size);
}

void
OfflineRequest::writeError(unsigned short status, const std::string &message) {
    *error_stream_ << status << std::endl << message << std::endl;
}

void
OfflineRequest::writeByWriter(const BinaryWriter *writer) {
    writer->write(data_stream_);
}

void
OfflineRequest::attach(const std::string &uri,
                       const std::string &xml,
                       const std::string &body,
                       const std::vector<std::string> &headers,
                       const std::vector<std::string> &vars,
                       std::ostream *data_stream,
                       std::ostream *error_stream) {
    
    data_stream_ = data_stream; 
    error_stream_ = error_stream;
    xml_ = xml;
    
    if (uri.empty()) {
        throw std::runtime_error("Cannot process empty uri");
    }
    
    std::string processed_url = Policy::instance()->getPathByScheme(this, uri);
    std::string root_directory = Policy::instance()->getRootByScheme(this, uri);
    
    std::vector<std::string> env(vars);
    bool need_method = true;
    bool need_docroot = true;
    bool is_post = false;
    Range root_range = createRange("DOCUMENT_ROOT=");
    Range method_range = createRange("REQUEST_METHOD=");
    for(std::vector<std::string>::const_iterator it = vars.begin();
        it != vars.end();
        ++it) {
        Range var = createRange(*it);
        if (need_docroot) {
            if (startsWith(var, root_range)) {
                need_docroot = false;
                if (root_directory.empty()) {
                    Range key, value;
                    split(var, '=', key, value);
                    root_directory = std::string(value.begin(), value.end());
                }
            }
        }
        if (need_method) {
            if (startsWith(var, method_range)) {
                Range key, value;
                split(var, '=', key, value);
                is_post = (value == createRange("POST"));
                need_method = false;
            }
        }

        if (!need_docroot && !need_method) {
            break;
        } 
    }
    
    if (need_method) {
        env.push_back(std::string(method_range.begin(), method_range.end()).append("GET"));
    }
    
    size_t query_pos = processed_url.rfind('?');
    if (query_pos != std::string::npos) {
        std::string query = processed_url.substr(query_pos + 1);
        if (!query.empty()) {
            env.push_back(std::string("QUERY_STRING=").append(query));
        }
        processed_url.erase(query_pos);
    }
    
    if (!root_directory.empty() && *root_directory.rbegin() != '/') {
        root_directory.push_back('/');
    }

    size_t base_pos = 0;
    size_t pos = processed_url.find("://");
    if (pos == std::string::npos) {
        if (processed_url[0] != '/') {
            processed_url = FileUtils::normalize(root_directory + processed_url);
        }
        else {
            root_directory = "/";
        }
    }
    else {
        std::string protocol = processed_url.substr(0, pos);
        if (strcasecmp(protocol.c_str(), "http") == 0) {
        }
        else if (strcasecmp(protocol.c_str(), "https") == 0) {
            env.push_back("HTTPS=on");
        }
        else {
            throw std::runtime_error("Protocol is not supported: " + protocol);
        }
        
        pos += 3;
        base_pos = processed_url.find('/', pos);
        std::string full_host = processed_url.substr(pos, base_pos - pos);

        unsigned short port_int;
        size_t port_pos = full_host.find(':');
        if (port_pos == std::string::npos) {
            port_int = 80;
        }
        else {
            std::string port = full_host.substr(port_pos + 1);
            try {
                port_int = boost::lexical_cast<unsigned short>(port);
            }
            catch(const boost::bad_lexical_cast &e) {
                throw std::runtime_error("Cannot parse port: " + port);
            }
        }
        
        if (!full_host.empty()) {
            env.push_back(std::string("SERVER_NAME=").append(full_host));
            env.push_back(std::string("SERVER_PORT=").append(boost::lexical_cast<std::string>(port_int)));
            env.push_back(std::string("HTTP_HOST=").append(full_host));
        }
        processed_url = FileUtils::normalize(root_directory + processed_url.substr(base_pos));
    }
    
    std::string script_name;
    size_t size = root_directory.size() - 1;
    if (size <= processed_url.size() &&
        strncmp(root_directory.c_str(), processed_url.c_str(), size) == 0) {
        script_name = processed_url.substr(size);
    }
    else {
        script_name = processed_url;
    }
    
    if (*root_directory.rbegin() == '/') {
        root_directory.erase(root_directory.size() - 1);
    }

    std::string script_file_name = FileUtils::normalize(root_directory + script_name);
    
    env.push_back(std::string("SCRIPT_NAME=").append(script_name));
    env.push_back(std::string("SCRIPT_FILENAME=").append(script_file_name));
    
    if (need_docroot && !root_directory.empty()) {
        env.push_back(std::string(root_range.begin(), root_range.end()).append(root_directory));
    }
    
    bool have_content_length = false;
    for(std::vector<std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        size_t eq_pos = it->find('=');
        if (eq_pos != std::string::npos) {
            std::string header("HTTP_");
            std::string header_name = StringUtils::toupper(it->substr(0, eq_pos));
            if (!have_content_length &&
                strncmp(header_name.c_str(), "CONTENT_LENGTH", sizeof("CONTENT_LENGTH")-1) == 0) {
                have_content_length = true;
            }
            header.append(header_name).append(it->substr(eq_pos));
            env.push_back(header);
        }
    }
    if (!have_content_length && is_post) {
        env.push_back(
            std::string("HTTP_CONTENT_LENGTH=").
                append(boost::lexical_cast<std::string>(body.size())));
    }
    
    char* env_vars[env.size() + 1];
    env_vars[env.size()] = NULL;
    for(unsigned int i = 0; i < env.size(); ++i) {
        env_vars[i] = const_cast<char*>(env[i].c_str());
    }

    std::stringstream stream(body);  
    DefaultRequestResponse::attach(&stream, env_vars);
}

void
OfflineRequest::detach() {
    DefaultRequestResponse::detach();
}

const std::string&
OfflineRequest::xml() const {
    return xml_;
}

} // namespace xscript
