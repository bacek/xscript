#include "settings.h"

#include <iostream>
#include <set>

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>

#include "standalone_request.h"
#include "xscript/logger.h"

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
    root_directory_ = doc_root;

    boost::regex expression(
        "^(\?:([^:/\?#]+)://)\?(\\w+[^/\?#:]*)\?(\?::(\\d+))\?(/\?(\?:[^\?#/]*/)*)\?([^\?#]*)\?(\\\?.*)\?");
    boost::cmatch matches;
    if (boost::regex_match(url.c_str(), matches, expression)) {
        std::string match(matches[1].first, matches[1].second);
        isSecure_ = false;
        if ("https" == std::string(matches[1].first, matches[1].second)) {
            isSecure_ = true;
        }

        host_ = std::string(matches[2].first, matches[2].second);
        match = std::string(matches[3].first, matches[3].second);
        port_ = 80;
        if (!match.empty()) {
            port_ = boost::lexical_cast<unsigned short>(match);
        }

        path_ = std::string(matches[4].first, matches[4].second);
        if (path_.empty()) {
            path_ = "/";
        }

        script_name_ = std::string(matches[5].first, matches[5].second);
        if (script_name_.empty()) {
            script_name_.swap(host_);
        }

        script_file_name_ = root_directory_ + path_ + script_name_;
        query_ = std::string(matches[6].first, matches[6].second).erase(0, 1);
    }
    else {
        throw std::runtime_error("Cannot parse url: " + url);
    }

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
