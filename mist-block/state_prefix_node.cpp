#include "settings.h"

#include <cassert>

#include "xscript/context.h"
#include "xscript/encoder.h"
#include "xscript/protocol.h"
#include "xscript/request.h"
#include "xscript/util.h"
#include "xscript/string_utils.h"

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

    const std::string& script_name = ctx->request()->getScriptName();
    if (!script_name.empty()) {
        setParameter(Protocol::PATH.c_str(), script_name);
    }

    const std::string& query_string = ctx->request()->getQueryString();
    if (!query_string.empty()) {
        setParameter(Protocol::QUERY.c_str(), query_string);
    }

    std::string uri = ctx->request()->getURI();
    if (!uri.empty()) {
        setParameter(Protocol::URI.c_str(), uri);
    }

    std::string originaluri = ctx->request()->getOriginalURI();
    if (!originaluri.empty()) {
        setParameter(Protocol::ORIGINAL_URI.c_str(), originaluri);
    }

    std::string originalurl = ctx->request()->getOriginalUrl();
    if (!originalurl.empty()) {
        setParameter(Protocol::ORIGINAL_URL.c_str(), originalurl);
    }

    std::string host = ctx->request()->getHost();
    if (!host.empty()) {
        setParameter(Protocol::HOST.c_str(), host);
    }

    std::string originalhost = ctx->request()->getOriginalHost();
    if (!originalhost.empty()) {
        setParameter(Protocol::ORIGINAL_HOST.c_str(), originalhost);
    }

    const std::string& path_info = ctx->request()->getPathInfo();
    if (!path_info.empty()) {
        setParameter(Protocol::PATH_INFO.c_str(), path_info);
    }

    const std::string& script_filename = ctx->request()->getScriptFilename();
    if (!script_filename.empty()) {
        setParameter(Protocol::REAL_PATH.c_str(), script_filename);
    }

    setParameter(Protocol::SECURE.c_str(), ctx->request()->isSecure() ? "yes" : "no");
    setParameter(Protocol::BOT.c_str(), ctx->request()->isBot() ? "yes" : "no");
    setParameter(Protocol::METHOD.c_str(), ctx->request()->getRequestMethod());

    const std::string& user = ctx->request()->getRemoteUser();
    if (!user.empty()) {
        setParameter(Protocol::HTTP_USER.c_str(), user);
    }

    const std::string& addr = ctx->request()->getRealIP();
    if (!addr.empty()) {
        setParameter(Protocol::REMOTE_IP.c_str(), addr);
    }

    int length = ctx->request()->getContentLength();
    if (length > 0) {
        setParameter(Protocol::CONTENT_LENGTH.c_str(), boost::lexical_cast<std::string>(length));
    }

    const std::string& enc = ctx->request()->getContentEncoding();
    if (!enc.empty()) {
        setParameter(Protocol::CONTENT_ENCODING.c_str(), enc);
    }

    const std::string& type = ctx->request()->getContentType();
    if (!type.empty()) {
        setParameter(Protocol::CONTENT_TYPE.c_str(), type);
    }
}


} // namespace xscript
