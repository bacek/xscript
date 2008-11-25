#include "settings.h"

#include <cassert>

#include "xscript/authorizer.h"
#include "xscript/context.h"
#include "xscript/encoder.h"
#include "xscript/protocol.h"
#include "xscript/request.h"
#include "xscript/util.h"

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
StateProtocolNode::build(Context* ctx) {

    const std::string& script_name = Protocol::getPathNative(ctx);
    if (!script_name.empty()) {
        setParameter(Protocol::PATH.c_str(), script_name);
    }

    const std::string& query_string = Protocol::getQueryNative(ctx);
    if (!query_string.empty()) {
        setParameter(Protocol::QUERY.c_str(), query_string);
    }

    std::string uri = Protocol::getURI(ctx);
    if (!uri.empty()) {
        setParameter(Protocol::URI.c_str(), uri);
    }

    std::string originaluri = Protocol::getOriginalURI(ctx);
    if (!originaluri.empty()) {
        setParameter(Protocol::ORIGINAL_URI.c_str(), originaluri);
    }

    std::string originalurl = Protocol::getOriginalUrl(ctx);
    if (!originalurl.empty()) {
        setParameter(Protocol::ORIGINAL_URL.c_str(), originalurl);
    }

    std::string host = Protocol::getHost(ctx);
    if (!host.empty()) {
        setParameter(Protocol::HOST.c_str(), host);
    }

    std::string originalhost = Protocol::getOriginalHost(ctx);
    if (!originalhost.empty()) {
        setParameter(Protocol::ORIGINAL_HOST.c_str(), originalhost);
    }

    const std::string& path_info = Protocol::getPathInfoNative(ctx);
    if (!path_info.empty()) {
        setParameter(Protocol::PATH_INFO.c_str(), path_info);
    }

    const std::string& script_filename = Protocol::getRealPathNative(ctx);
    if (!script_filename.empty()) {
        setParameter(Protocol::REAL_PATH.c_str(), script_filename);
    }

    setParameter(Protocol::SECURE.c_str(), Protocol::getSecure(ctx));
    setParameter(Protocol::METHOD.c_str(), Protocol::getMethodNative(ctx));

    const std::string& user = Protocol::getHttpUserNative(ctx);
    if (!user.empty()) {
        setParameter(Protocol::HTTP_USER.c_str(), user);
    }

    const std::string& addr = Protocol::getRemoteIPNative(ctx);
    if (!addr.empty()) {
        setParameter(Protocol::REMOTE_IP.c_str(), addr);
    }

    std::string content_length = Protocol::getContentLength(ctx);
    if (!content_length.empty() && content_length[0] != '0' && content_length[0] != '-') {
        setParameter(Protocol::CONTENT_LENGTH.c_str(), content_length);
    }

    const std::string& enc = Protocol::getContentEncodingNative(ctx);
    if (!enc.empty()) {
        setParameter(Protocol::CONTENT_ENCODING.c_str(), enc);
    }

    const std::string& type = Protocol::getContentTypeNative(ctx);
    if (!type.empty()) {
        setParameter(Protocol::CONTENT_TYPE.c_str(), type);
    }

    setParameter(Protocol::BOT.c_str(), Protocol::getBot(ctx));
}


} // namespace xscript
