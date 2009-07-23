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

OfflineRequest::OfflineRequest(const std::string &docroot) :
    data_stream_(NULL), error_stream_(NULL), docroot_(docroot), need_output_(true) {
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
OfflineRequest::processHeaders(const std::vector<std::string> &headers,
                               unsigned long body_size,
                               std::vector<std::string> &env) {
    bool have_content_length = false;
    for(std::vector<std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {      
        Range var = createRange(*it);
        Range key, value;
        split(var, '=', key, value);
        if (value.empty()) {
            continue;
        }
        
        std::string header_name = StringUtils::toupper(std::string(key.begin(), key.end()));
        std::replace(header_name.begin(), header_name.end(), '-', '_');
        
        std::string header("HTTP_");
        if (!have_content_length &&
            strncmp(header_name.c_str(), "CONTENT_LENGTH", sizeof("CONTENT_LENGTH")-1) == 0) {
            have_content_length = true;
        }
        
        header.append(header_name);
        header.push_back('=');
        header.append(std::string(value.begin(), value.end()));
        env.push_back(header);
    }
    
    if (!have_content_length && body_size) {
        env.push_back(std::string("HTTP_CONTENT_LENGTH=").
                append(boost::lexical_cast<std::string>(body_size)));
    }
}

void
OfflineRequest::processVariables(const std::vector<std::string> &vars,
                                 std::vector<std::string> &env) {
  
    Range method = createRange("GET");
    for(std::vector<std::string>::const_iterator it = vars.begin(); it != vars.end(); ++it) {      
        Range var = createRange(*it);
        Range key, value;
        split(var, '=', key, value);
        if (value.empty()) {
            continue;
        }
        
        if (key == createRange("DOCUMENT_ROOT")) {
            docroot_ = std::string(value.begin(), value.end());
        }
        else if (key == createRange("REQUEST_METHOD")) {
            method = value;
        }
        else {
            env.push_back(*it);
        }
    }
    
    if (*docroot_.rbegin() == '/') {
        docroot_.erase(docroot_.size() - 1);
    }
    
    env.push_back(std::string("DOCUMENT_ROOT=").append(docroot_));
    env.push_back(std::string("REQUEST_METHOD=").append(method.begin(), method.end())); 
}

void
OfflineRequest::attach(const std::string &uri,
                       const std::string &xml,
                       const std::string &body,
                       const std::vector<std::string> &headers,
                       const std::vector<std::string> &vars,
                       std::ostream *data_stream,
                       std::ostream *error_stream,
                       bool need_output) {
    try {
        data_stream_ = data_stream; 
        error_stream_ = error_stream;
        xml_ = xml;
        need_output_ = need_output;
        
        if (uri.empty()) {
            throw std::runtime_error("Cannot process empty uri");
        }
        
        std::vector<std::string> env;
        processVariables(vars, env);
        processHeaders(headers, body.size(), env);
    
        std::string processed_url = Policy::getPathByScheme(this, uri);
        
        size_t query_pos = processed_url.rfind('?');
        if (query_pos != std::string::npos) {
            std::string query = processed_url.substr(query_pos + 1);
            if (!query.empty()) {
                env.push_back(std::string("QUERY_STRING=").append(query));
            }
            processed_url.erase(query_pos);
        }
        
        size_t pos = processed_url.find("://");
        if (std::string::npos == pos) {
            if (processed_url[0] != '/') {
                processed_url = docroot_ + "/" + processed_url;
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
            size_t base_pos = processed_url.find('/', pos);
            
            std::string full_host;
            unsigned short port_int = 80;
            if (std::string::npos == base_pos) {
                processed_url.erase(0, pos);
            }
            else {
                full_host = processed_url.substr(pos, base_pos - pos);
        
                size_t port_pos = full_host.find(':');
                if (std::string::npos != port_pos) {
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
                processed_url.erase(0, base_pos);
            }
    
            processed_url = docroot_ + "/" + processed_url;
        }
    
        std::string script_file_name = FileUtils::normalize(processed_url);
        std::string script_name;
        
        size_t size = docroot_.size();
        if (size <= script_file_name.size() &&
            strncmp(docroot_.c_str(), processed_url.c_str(), size) == 0) {
            script_name = script_file_name.substr(size);
        }
        else {
            script_name = script_file_name;
        }
    
        env.push_back(std::string("SCRIPT_NAME=").append(script_name));
        env.push_back(std::string("SCRIPT_FILENAME=").append(script_file_name));
        
        char* env_vars[env.size() + 1];
        env_vars[env.size()] = NULL;
        for(unsigned int i = 0; i < env.size(); ++i) {
            env_vars[i] = const_cast<char*>(env[i].c_str());
        }
    
        std::stringstream stream(body);  
        DefaultRequestResponse::attach(&stream, env_vars);
    }
    catch(const BadRequestError &e) {
        throw;
    }
    catch(const std::exception &e) {
        throw BadRequestError(e.what());
    }
    catch(...) {
        throw BadRequestError("Unknown error");
    }
}

const std::string&
OfflineRequest::getDocumentRoot() const {
    return docroot_;
}

bool
OfflineRequest::suppressBody() const {
    return need_output_ ? DefaultRequestResponse::suppressBody() : true;
}

const std::string&
OfflineRequest::xml() const {
    return xml_;
}

} // namespace xscript
