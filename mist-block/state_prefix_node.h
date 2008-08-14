#ifndef _XSCRIPT_MIST_STATE_PREFIX_NODE_H_
#define _XSCRIPT_MIST_STATE_PREFIX_NODE_H_

#include <cassert>
#include <string>

#include "state_node.h"
#include "xscript/state.h"

namespace xscript {
class Request;
class Encoder;

class StatePrefixNode : public StateNode {
public:
    StatePrefixNode(const std::string& prefix, const char* type_str, State* state);

    void setParameter(const char* name, const std::string& val);
    void setParameters(const char* name, const std::vector<std::string>& v);

private:
    const std::string& prefix_;
    State* state_;
};


class StateQueryNode : public StatePrefixNode {
public:
    StateQueryNode(const std::string& prefix, State* state);

    void build(const std::string& query);
};


class StateRequestNode : public StatePrefixNode {
public:
    StateRequestNode(const std::string& prefix, State* state);

    void build(const Request* req, bool urlencode, Encoder* encoder);
};


class StateHeadersNode : public StatePrefixNode {
public:
    StateHeadersNode(const std::string& prefix, State* state);

    void build(const Request* req);
};


class StateCookiesNode : public StatePrefixNode {
public:
    StateCookiesNode(const std::string& prefix, State* state);

    void build(const Request* req);
};


class StateProtocolNode : public StatePrefixNode {
public:
    StateProtocolNode(const std::string& prefix, State* state);

    void build(const Request* req);
};


} // namespace xscript

#endif // _XSCRIPT_MIST_STATE_PREFIX_NODE_H_
