#include "settings.h"

#include <cassert>

#include "xscript/encoder.h"
#include "xscript/util.h"
#include "xscript/request.h"
#include "state_param_node.h"
#include "state_prefix_node.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

StatePrefixNode::StatePrefixNode(const std::string& prefix, const char* type_str, State* state) :
        StateNode() , prefix_(prefix) , state_(state) {
    setProperty("prefix", prefix_.c_str());
    setType(type_str);
}

void
StatePrefixNode::setParameter(const char* name, const std::string& val) {
    StateParamNode stateParamNode(getNode(), name);
    stateParamNode.createSubNode(val.c_str());

    if (NULL != state_) {
        state_->setString(prefix_ + name, val);
    }
}

void
StatePrefixNode::setParameters(const char* name, const std::vector<std::string>& v) {
    StateParamNode stateParamNode(getNode(), name);
    stateParamNode.createSubNodes(v);

    if (NULL != state_) {
        std::string val;
        bool is_first = true;
        for (std::vector<std::string>::const_iterator i = v.begin(), jend = v.end(); i != jend; ++i) {
            if ( is_first ) {
                is_first = false;
            }
            else {
                val += ",";
            }
            val += *i;
        }
        state_->setString(prefix_ + name, val);
    }
}


StateQueryNode::StateQueryNode(const std::string& prefix, State* state) :
        StatePrefixNode(prefix, "Query", state) {
}

void
StateQueryNode::build(const std::string& query) {
    std::vector<StringUtils::NamedValue> params;

    std::string::size_type i = query.find("&amp;");
    if (i == std::string::npos) {
        StringUtils::parse(query, params);
    }
    else {
        std::string q = query;
        for (;;) {
            ++i; // set position to "amp;"
            q.erase(i, 4); //
            i = q.find("&amp;", i);
            if (i == std::string::npos) {
                break;
            }
        }
        StringUtils::parse(q, params);
    }

    for (std::vector<StringUtils::NamedValue>::const_iterator i = params.begin(), end = params.end(); i != end; ++i) {
        setParameter(i->first.c_str(), i->second);
    }
}


StateRequestNode::StateRequestNode(const std::string& prefix, State* state) :
        StatePrefixNode(prefix, "Request", state) {
}

void
StateRequestNode::build(const Request* req, bool urlencode, Encoder* encoder) {
    if (NULL != req && req->countArgs() > 0) {
        std::vector<std::string> names;
        req->argNames(names);
        for (std::vector<std::string>::const_iterator i = names.begin(), end = names.end(); i != end; ++i) {
            std::string name = *i;

            std::vector<std::string> values;
            req->getArg(name, values);
            assert(values.size() > 0);

            if (encoder) {
                for (std::vector<std::string>::iterator it = values.begin(); it != values.end(); ++it) {
                    *it = encoder->encode(*it);
                }
                name = encoder->encode(name);
            }

            if (urlencode) {
                for (std::vector<std::string>::iterator it = values.begin(); it != values.end(); ++it) {
                    *it = StringUtils::urlencode(*it);
                }
                name = StringUtils::urlencode(name);
            }

            if (values.size() == 1) {
                setParameter(name.c_str(), values[0]);
            }
            else {
                setParameters(name.c_str(), values);
            }
        }
    }
}


StateHeadersNode::StateHeadersNode(const std::string& prefix, State* state) :
        StatePrefixNode(prefix, "Headers", state) {
}

void
StateHeadersNode::build(const Request* req) {
    if (NULL != req && req->countHeaders() > 0) {
        std::vector<std::string> names;
        req->headerNames(names);
        for (std::vector<std::string>::const_iterator i = names.begin(), end = names.end(); i != end; ++i) {
            const std::string& name = *i;

            const std::string& header = req->getHeader(name);
            setParameter(name.c_str(), header);
        }
    }
}


StateCookiesNode::StateCookiesNode(const std::string& prefix, State* state) :
        StatePrefixNode(prefix, "Cookies", state) {
}

void
StateCookiesNode::build(const Request* req) {
    if (NULL != req && req->countCookies() > 0) {
        std::vector<std::string> names;
        req->cookieNames(names);
        for (std::vector<std::string>::const_iterator i = names.begin(), end = names.end(); i != end; ++i) {
            const std::string& name = *i;

            const std::string& cookie = req->getCookie(name);
            setParameter(name.c_str(), cookie);
        }
    }
}


StateProtocolNode::StateProtocolNode(const std::string& prefix, State* state) :
        StatePrefixNode(prefix, "Protocol", state) {
}

void
StateProtocolNode::build(const Request* req) {

    if (NULL != req) {
        const std::string& script_name = req->getScriptName();
        if (!script_name.empty()) {
            setParameter("path", script_name);
        }

        const std::string& query_string = req->getQueryString();
        if (!query_string.empty()) {
            setParameter("query", query_string);
        }

        std::string uri = req->getURI();
        if (!uri.empty()) {
            setParameter("uri", uri);
        }

        std::string originaluri = req->getOriginalURI();
        if (!originaluri.empty()) {
            setParameter("originaluri", originaluri);
        }

        std::string originalurl = req->getOriginalUrl();
        if (!originalurl.empty()) {
            setParameter("originalurl", originalurl);
        }

        std::string host = req->getHost();
        if (!host.empty()) {
            setParameter("host", host);
        }

        std::string originalhost = req->getOriginalHost();
        if (!originalhost.empty()) {
            setParameter("originalhost", originalhost);
        }

        const std::string& path_info = req->getPathInfo();
        if (!path_info.empty()) {
            setParameter("pathinfo", path_info);
        }

        const std::string& script_filename = req->getScriptFilename();
        if (!script_filename.empty()) {
            setParameter("realpath", script_filename);
        }

        setParameter("secure", req->isSecure() ? "yes" : "no");
        setParameter("method", req->getRequestMethod());

        const std::string& user = req->getRemoteUser();
        if (!user.empty()) {
            setParameter("http_user", user);
        }

        const std::string& addr = req->getRealIP();
        if (!addr.empty()) {
            setParameter("remote_ip", addr);
        }

        std::streamsize length = req->getContentLength();
        if (length > 0) {
            std::string length_str = boost::lexical_cast<std::string>(length);
            if (!length_str.empty()) {
                setParameter("content-length", length_str);
            }
        }

        const std::string& enc = req->getContentEncoding();
        if (!enc.empty()) {
            setParameter("content-encoding", enc);
        }

        const std::string& type = req->getContentType();
        if (!type.empty()) {
            setParameter("content-type", type);
        }
    }
}


} // namespace xscript
