#include "settings.h"

#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/current_function.hpp>

#include <libxml/xmlstring.h>

#include "mist_block.h"
#include "state_node.h"
#include "state_prefix_node.h"

#include "xscript/util.h"
#include "xscript/xml.h"
#include "xscript/xml_util.h"
#include "xscript/state.h"
#include "xscript/param.h"
#include "xscript/logger.h"
#include "xscript/context.h"
#include "xscript/encoder.h"
#include "xscript/response.h"
#include "xscript/profiler.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

// NB: Multiline macro for function prologue
#define PROLOGUE                                                         \
    PROFILER(                                                       \
        log(),                                                          \
        std::string(BOOST_CURRENT_FUNCTION) + ", " + owner()->name()    \
    );  \
    log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());

namespace xscript {

class MistMethodRegistrator {
public:
    MistMethodRegistrator();
};

MethodMap MistBlock::methods_;

MistBlock::MistBlock(const Extension *ext, Xml *owner, xmlNodePtr node) :
        Block(ext, owner, node), method_(NULL) {
}

MistBlock::~MistBlock() {
}

void
MistBlock::postParse() {

    Block::postParse();
    MethodMap::iterator i = methods_.find(method());
    if (methods_.end() == i) {
        std::stringstream stream;
        stream << "nonexistent mist method call: " << method();
        throw std::invalid_argument(stream.str());
    }
    else {
        method_ = i->second;
    }
}

XmlDocHelper
MistBlock::call(Context *ctx, boost::any &) throw (std::exception) {

    XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != doc.get());

    xmlDocSetRootElement(doc.get(), (this->*method_)(ctx));
    return doc;
}

xmlNodePtr
MistBlock::setStateLong(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (2 != p.size()) {
        throw InvokeError("bad arity");
    }
    State* state = ctx->state();
    std::string n = p[0]->asString(ctx), val = p[1]->asString(ctx);
    boost::int32_t value = 0;
    try {
        value = boost::lexical_cast<boost::int32_t>(val);
    }
    catch (const boost::bad_lexical_cast &e) {
        value = 0;
    }

    state->checkName(n);
    state->setLong(n, value);

    StateNode node("long", n.c_str(), boost::lexical_cast<std::string>(value).c_str());
    return node.releaseNode();
}

xmlNodePtr
MistBlock::setStateString(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (2 != p.size()) {
        throw InvokeError("bad arity");
    }

    State* state = ctx->state();
    std::string n = p[0]->asString(ctx), val = p[1]->asString(ctx);

    state->checkName(n);
    state->setString(n, val);

    StateNode node("string", n.c_str(), XmlUtils::escape(val).c_str());
    return node.releaseNode();
}

xmlNodePtr
MistBlock::setStateDouble(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (2 != p.size()) {
        throw InvokeError("bad arity");
    }
    State* state = ctx->state();
    std::string n = p[0]->asString(ctx), val = p[1]->asString(ctx);
    double value = 0.0;
    try {
        value = boost::lexical_cast<double>(val);
    }
    catch (const boost::bad_lexical_cast &e) {
        value = 0.0;
    }

    state->checkName(n);
    state->setDouble(n, value);

    StateNode node("double", n.c_str(), boost::lexical_cast<std::string>(value).c_str());
    return node.releaseNode();
}

xmlNodePtr
MistBlock::setStateLongLong(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (2 != p.size()) {
        throw InvokeError("bad arity");
    }
    State* state = ctx->state();
    std::string n = p[0]->asString(ctx), val = p[1]->asString(ctx);
    boost::int64_t value = 0;
    try {
        value = boost::lexical_cast<boost::int64_t>(val);
    }
    catch (const boost::bad_lexical_cast &e) {
        value = 0;
    }

    state->checkName(n);
    state->setLongLong(n, value);

    StateNode node("longlong", n.c_str(), boost::lexical_cast<std::string>(value).c_str());
    return node.releaseNode();
}

xmlNodePtr
MistBlock::setStateRandom(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (3 != p.size()) {
        throw InvokeError("bad arity");
    }
    std::string n = p[0]->asString(ctx);

    State* state = ctx->state();
    state->checkName(n);

    boost::int64_t lo;
    try {
        lo = boost::lexical_cast<boost::int64_t>(p[1]->asString(ctx));
    }
    catch (const boost::bad_lexical_cast &e) {
        throw std::invalid_argument("bad param: lo");
    }

    boost::int64_t hi;
    try {
        hi = boost::lexical_cast<boost::int64_t>(p[2]->asString(ctx));
    }
    catch (const boost::bad_lexical_cast &e) {
        throw std::invalid_argument("bad param: hi");
    }

    if (hi <= lo) {
        throw std::invalid_argument("bad range");
    }

    boost::int64_t val = lo;
    if (RAND_MAX < hi - lo) {
        log()->warn("too wide range in mist:set_state_random");
        hi = lo + RAND_MAX;
        val += random();
    }
    else if (RAND_MAX == hi - lo) {
        val += random();
    }
    else {
        val += random() % (hi - lo);
    }

    state->setLongLong(n, val);

    StateNode node("random", n.c_str(), boost::lexical_cast<std::string>(val).c_str());
    return node.releaseNode();
}

xmlNodePtr
MistBlock::setStateDefined(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (2 != p.size()) {
        throw InvokeError("bad arity");
    }

    std::string n = p[0]->asString(ctx);

    State* state = ctx->state();
    state->checkName(n);

    typedef boost::char_separator<char> Separator;
    typedef boost::tokenizer<Separator> Tokenizer;

    std::string names = p[1]->asString(ctx), val;
    Tokenizer tok(names, Separator(","));
    for (Tokenizer::iterator i = tok.begin(), end = tok.end(); i != end; ++i) {
        if (state->has(*i) && !state->asString(*i).empty()) {
            state->copy(*i, n);
            val = state->asString(n);
            break;
        }
    }

    StateNode node("defined", n.c_str(), XmlUtils::escape(val).c_str());
    return node.releaseNode();
}

xmlNodePtr
MistBlock::setStateUrlencode(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (2 != p.size() && 3 != p.size()) {
        throw InvokeError("bad arity");
    }

    State* state = ctx->state();
    std::string n = p[0]->asString(ctx), val = p[1]->asString(ctx);
    state->checkName(n);

    if (3 == p.size()) {
        std::string encoding = p[2]->asString(ctx);
        std::auto_ptr<Encoder> encoder = Encoder::createEscaping("utf-8", encoding.c_str());
        val = encoder->encode(val);
    }
    val = StringUtils::urlencode(val);

    state->setString(n, val);

    StateNode node("urlencode", n.c_str(), val.c_str());
    return node.releaseNode();
}

xmlNodePtr
MistBlock::setStateUrldecode(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (2 != p.size() && 3 != p.size()) {
        throw InvokeError("bad arity");
    }

    State* state = ctx->state();
    std::string n = p[0]->asString(ctx), val = p[1]->asString(ctx);
    state->checkName(n);

    val = StringUtils::urldecode(val);
    if (3 == p.size()) {
        std::string encoding = p[2]->asString(ctx);
        std::auto_ptr<Encoder> encoder = Encoder::createEscaping(encoding.c_str(), "utf-8");
        val = encoder->encode(val);
    }

    state->setString(n, val);

    StateNode node("urldecode", n.c_str(), XmlUtils::escape(val).c_str());
    return node.releaseNode();
}

xmlNodePtr
MistBlock::setStateByKeys(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (4 != p.size()) {
        throw InvokeError("bad arity");
    }

    State* state = ctx->state();
    std::string n = p[0]->asString(ctx);
    state->checkName(n);

    std::string keys = p[1]->asString(ctx);
    std::string vals = p[2]->asString(ctx);
    std::string input = p[3]->asString(ctx);

    typedef boost::char_separator<char> Separator;
    typedef boost::tokenizer<Separator> Tokenizer;

    Separator sep(",");
    Tokenizer keytok(keys, sep), valtok(vals, sep);

    Tokenizer::iterator ki = keytok.begin(), kend = keytok.end();
    Tokenizer::iterator vi = valtok.begin(), vend = valtok.end();

    std::map<std::string, std::string> m;
    for ( ; ki != kend && vi != vend; ++ki, ++vi) {
        m.insert(std::make_pair(*ki, *vi));
    }

    std::string res;
    Tokenizer tok(input, sep);
    for (Tokenizer::iterator i = tok.begin(), end = tok.end(); i != end; ++i) {
        std::map<std::string, std::string>::iterator mi = m.find(*i);
        if (m.end() != mi && !mi->second.empty()) {
            res = mi->second;
            state->setString(n, res);
            break;
        }
    }

    StateNode node("keys", n.c_str(), XmlUtils::escape(res).c_str());
    return node.releaseNode();
}

xmlNodePtr
MistBlock::setStateByDate(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (1 != p.size()) {
        throw InvokeError("bad arity");
    }

    State* state = ctx->state();
    std::string name = p[0]->asString(ctx);
    state->checkName(name);
    std::string name_timestamp = name + std::string("_timestamp");
    state->checkName(name_timestamp);

    time_t now_seconds = time(NULL);
    const char* date_format_iso = "%Y-%m-%d";

    char buf[32];
    struct tm ttm;
    memset(buf, 0, sizeof(buf));

    localtime_r(&now_seconds, &ttm);
    strftime(buf, sizeof(buf), date_format_iso, &ttm);

    std::string now_str(buf);
    state->setString(name, now_str);
    std::string timestamp_str = boost::lexical_cast<std::string>(now_seconds);
    state->setLongLong(name_timestamp, now_seconds);

    StateNode node("date", name.c_str(), now_str.c_str());

    strftime(buf, sizeof(buf), "%z", &ttm);
    node.setProperty("zone", buf);

    strftime(buf, sizeof(buf), "%u", &ttm);
    node.setProperty("weekday", buf);

    node.setProperty("timestamp", timestamp_str.c_str());

    now_seconds -= 86400; // seconds in one day

    localtime_r(&now_seconds, &ttm);
    strftime(buf, sizeof(buf), date_format_iso, &ttm);
    node.setProperty("before", buf);

    return node.releaseNode();
}

xmlNodePtr
MistBlock::setStateByQuery(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (2 != p.size()) {
        throw InvokeError("bad arity");
    }

    std::string prefix = p[0]->asString(ctx);
    std::string query = p[1]->asString(ctx);

    State* state = ctx->state();

    StateQueryNode node(prefix, state);
    node.build(query);
    return node.releaseNode();
}

xmlNodePtr
MistBlock::echoQuery(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (2 != p.size()) {
        throw InvokeError("bad arity");
    }

    std::string prefix = p[0]->asString(ctx);
    std::string query = p[1]->asString(ctx);

    StateQueryNode node(prefix, NULL);
    node.build(query);
    return node.releaseNode();
}

xmlNodePtr
MistBlock::setStateByRequest(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (1 != p.size()) {
        throw InvokeError("bad arity");
    }

    std::string prefix = p[0]->asString(ctx);

    State* state = ctx->state();

    StateRequestNode node(prefix, state);
    node.build(ctx->request(), false, NULL);
    return node.releaseNode();
}

xmlNodePtr
MistBlock::setStateByRequestUrlencoded(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (p.size() < 1 || p.size() > 2) {
        throw InvokeError("bad arity");
    }

    std::string prefix = p[0]->asString(ctx);

    std::auto_ptr<Encoder> encoder(NULL);
    if (2 == p.size()) {
        std::string encoding = p[1]->asString(ctx);
        if (strncasecmp(encoding.c_str(), "utf-8", sizeof("utf-8") - 1) != 0) {
            encoder = std::auto_ptr<Encoder>(Encoder::createEscaping("utf-8", encoding.c_str()));
        }
    }

    State* state = ctx->state();

    StateRequestNode node(prefix, state);
    node.build(ctx->request(), true, encoder.get());
    return node.releaseNode();
}

xmlNodePtr
MistBlock::echoRequest(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (1 != p.size()) {
        throw InvokeError("bad arity");;
    }

    std::string prefix = p[0]->asString(ctx);

    StateRequestNode node(prefix, NULL);
    node.build(ctx->request(), false, NULL);
    return node.releaseNode();
}

xmlNodePtr
MistBlock::setStateByHeaders(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (1 != p.size()) {
        throw InvokeError("bad arity");
    }

    std::string prefix = p[0]->asString(ctx);

    State* state = ctx->state();

    StateHeadersNode node(prefix, state);
    node.build(ctx->request());
    return node.releaseNode();
}

xmlNodePtr
MistBlock::echoHeaders(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (1 != p.size()) {
        throw InvokeError("bad arity");
    }

    std::string prefix = p[0]->asString(ctx);

    StateHeadersNode node(prefix, NULL);
    node.build(ctx->request());
    return node.releaseNode();
}

xmlNodePtr
MistBlock::setStateByCookies(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (1 != p.size()) {
        throw InvokeError("bad arity");
    }

    std::string prefix = p[0]->asString(ctx);

    State* state = ctx->state();

    StateCookiesNode node(prefix, state);
    node.build(ctx->request());
    return node.releaseNode();
}

xmlNodePtr
MistBlock::echoCookies(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (1 != p.size()) {
        throw InvokeError("bad arity");
    }

    std::string prefix = p[0]->asString(ctx);

    StateCookiesNode node(prefix, NULL);
    node.build(ctx->request());
    return node.releaseNode();
}

xmlNodePtr
MistBlock::setStateByProtocol(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (1 != p.size()) {
        throw InvokeError("bad arity");
    }

    std::string prefix = p[0]->asString(ctx);

    State* state = ctx->state();

    StateProtocolNode node(prefix, state);
    node.build(ctx->request());
    return node.releaseNode();
}

xmlNodePtr
MistBlock::echoProtocol(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (1 != p.size()) {
        throw InvokeError("bad arity");
    }

    std::string prefix = p[0]->asString(ctx);

    StateProtocolNode node(prefix, NULL);
    node.build(ctx->request());
    return node.releaseNode();
}

xmlNodePtr
MistBlock::setStateJoinString(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (3 != p.size()) {
        throw InvokeError("bad arity");
    }

    std::string n = p[0]->asString(ctx);
    State* state = ctx->state();
    state->checkName(n);

    std::vector<std::string> keys;
    std::string prefix = p[1]->asString(ctx), delim = p[2]->asString(ctx);

    state->keys(keys);
    std::map<unsigned int, std::string> m;
    for (std::vector<std::string>::iterator i = keys.begin(), end = keys.end(); i != end; ++i) {
        if (i->find(prefix) == 0) {
            std::string num = i->substr(prefix.size(), std::string::npos);
            try {
                unsigned int n = boost::lexical_cast<unsigned int>(num);
                m.insert(std::make_pair(n, state->asString(*i)));
            }
            catch (const boost::bad_lexical_cast &e) {
            }
        }
    }

    std::string val;
    for (std::map<unsigned int, std::string>::iterator i = m.begin(), end = m.end(); i != end; ) {
        val.append(i->second);
        if (end != ++i) {
            val.append(delim);
        }
    }
    state->setString(n, val);

    StateNode node("join_string", n.c_str(), XmlUtils::escape(val).c_str());
    return node.releaseNode();
}

xmlNodePtr
MistBlock::setStateSplitString(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (3 != p.size()) {
        throw InvokeError("bad arity");
    }

    std::string prefix = p[0]->asString(ctx);
    State* state = ctx->state();
    state->checkName(prefix);

    std::string val = p[1]->asString(ctx), delim = p[2]->asString(ctx);

    StatePrefixNode node(prefix, "split_string", state);

    bool searching = true;
    unsigned int count = 0;
    std::string::size_type pos, lpos = 0;
    while (searching) {

        if ((pos = val.find(delim, lpos)) == std::string::npos) {
            searching = false;
        }
        std::string stateval = val.substr(lpos, searching ? pos - lpos : std::string::npos);
        lpos = pos + delim.size();

        std::string num = boost::lexical_cast<std::string>(count++);
        state->setString(prefix + num, stateval);

        XmlChildNode child(node.getNode(), "part", stateval.c_str());
        child.setProperty("no", num.c_str());
    }
    return node.releaseNode();
}

xmlNodePtr
MistBlock::setStateConcatString(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (p.size() < 3) {
        throw InvokeError("bad arity");
    }

    std::string n = p[0]->asString(ctx), val;
    State* state = ctx->state();
    state->checkName(n);

    for (std::vector<std::string>::size_type i = 1; i < p.size(); ++i) {
        val.append(p[i]->asString(ctx));
    }

    state->setString(n, val);

    StateNode node("concat_string", n.c_str(), XmlUtils::escape(val).c_str());
    return node.releaseNode();
}

xmlNodePtr
MistBlock::dropState(Context *ctx) {

    PROLOGUE;

    std::string prefix;
    const std::vector<Param*> &p = params();
    if (p.size() > 1) {
        throw InvokeError("bad arity");
    }
    if (1 == p.size()) {
        prefix = p[0]->asString(ctx);
    }

    State* state = ctx->state();
    if (prefix.empty()) {
        state->clear();
    }
    else {
        state->erasePrefix(prefix);
    }

    StatePrefixNode node(prefix, "drop", NULL);
    return node.releaseNode();
}

xmlNodePtr
MistBlock::dumpState(Context *ctx) {
    PROLOGUE;

    State* state = ctx->state();

    XmlNode node("state_dump");

    std::map<std::string, StateValue> state_info;
    state->values(state_info);

    for (std::map<std::string, StateValue>::const_iterator it = state_info.begin();
            it != state_info.end(); ++it) {
        XmlChildNode child(node.getNode(), "param", it->second.value().c_str());
        child.setProperty("name", it->first.c_str());
        child.setProperty("type", it->second.stringType().c_str());
    }
    return node.releaseNode();
}

xmlNodePtr
MistBlock::attachStylesheet(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (1 != p.size()) {
        throw InvokeError("bad arity");
    }
    std::string name = p[0]->asString(ctx);
    ctx->xsltName(name);

    XmlNode node("stylesheet");
    node.setType("attach");
    node.setContent(name.c_str());
    return node.releaseNode();
}

xmlNodePtr
MistBlock::location(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (1 != p.size()) {
        throw InvokeError("bad arity");
    }
    std::string location = p[0]->asString(ctx);
    ctx->response()->setStatus(302);
    ctx->response()->setHeader("Location", location);

    XmlNode node("location");
    node.setContent(location.c_str());
    return node.releaseNode();
}

xmlNodePtr
MistBlock::setStatus(Context *ctx) {

    PROLOGUE;

    const std::vector<Param*> &p = params();
    if (1 != p.size()) {
        throw InvokeError("bad arity");
    }

    std::string val = p[0]->asString(ctx);
    boost::int32_t status = 0;
    try {
        status = boost::lexical_cast<boost::int32_t>(val);
    }
    catch (const boost::bad_lexical_cast &e) {
        throw std::runtime_error("unknown status");
    }

    ctx->response()->setStatus(status);

    XmlNode node("status");
    node.setContent(val.c_str());
    return node.releaseNode();
}

void
MistBlock::registerMethod(const char *name, MistMethod method) {

    try {
        std::string n(name);
        std::pair<std::string, MistMethod> p(n, method);

        MethodMap::iterator i = methods_.find(n);
        if (methods_.end() == i) {
            methods_.insert(p);
        }
        else {
            std::stringstream stream;
            stream << "registering duplicate mist method: " << n;
            throw std::invalid_argument(stream.str());
        }
    }
    catch (const std::exception &e) {
        xscript::log()->error("%s, caught exception: %s",  BOOST_CURRENT_FUNCTION, e.what());
        throw;
    }
}



MistExtension::MistExtension() {
}

MistExtension::~MistExtension() {
}

const char*
MistExtension::name() const {
    return "mist";
}

const char*
MistExtension::nsref() const {
    return XmlUtils::XSCRIPT_NAMESPACE;
}

void
MistExtension::initContext(Context *ctx) {
    (void)ctx;
}

void
MistExtension::stopContext(Context *ctx) {
    (void)ctx;
}

void
MistExtension::destroyContext(Context *ctx) {
    (void)ctx;
}

std::auto_ptr<Block>
MistExtension::createBlock(Xml *owner, xmlNodePtr node) {
    return std::auto_ptr<Block>(new MistBlock(this, owner, node));
}

void
MistExtension::init(const Config *config) {
    (void)config;
}

MistMethodRegistrator::MistMethodRegistrator() {

    MistBlock::registerMethod("setStateLong", &MistBlock::setStateLong);
    MistBlock::registerMethod("set_state_long", &MistBlock::setStateLong);

    MistBlock::registerMethod("setStateString", &MistBlock::setStateString);
    MistBlock::registerMethod("set_state_string", &MistBlock::setStateString);

    MistBlock::registerMethod("setStateDouble", &MistBlock::setStateDouble);
    MistBlock::registerMethod("set_state_double", &MistBlock::setStateDouble);

    MistBlock::registerMethod("setStateLonglong", &MistBlock::setStateLongLong);
    MistBlock::registerMethod("set_state_longlong", &MistBlock::setStateLongLong);

    MistBlock::registerMethod("setStateRandom", &MistBlock::setStateRandom);
    MistBlock::registerMethod("set_state_random", &MistBlock::setStateRandom);

    MistBlock::registerMethod("setStateDefined", &MistBlock::setStateDefined);
    MistBlock::registerMethod("set_state_defined", &MistBlock::setStateDefined);

    MistBlock::registerMethod("set_state_urlencode", &MistBlock::setStateUrlencode);
    MistBlock::registerMethod("setStateUrlencode", &MistBlock::setStateUrlencode);

    MistBlock::registerMethod("set_state_urldecode", &MistBlock::setStateUrldecode);
    MistBlock::registerMethod("setStateUrldecode", &MistBlock::setStateUrldecode);

    MistBlock::registerMethod("setStateByKey", &MistBlock::setStateByKeys);
    MistBlock::registerMethod("set_state_by_key", &MistBlock::setStateByKeys);

    MistBlock::registerMethod("setStateByKeys", &MistBlock::setStateByKeys);
    MistBlock::registerMethod("set_state_by_keys", &MistBlock::setStateByKeys);

    MistBlock::registerMethod("setStateByDate", &MistBlock::setStateByDate);
    MistBlock::registerMethod("set_state_by_date", &MistBlock::setStateByDate);

    MistBlock::registerMethod("setStateByQuery", &MistBlock::setStateByQuery);
    MistBlock::registerMethod("set_state_by_query", &MistBlock::setStateByQuery);

    MistBlock::registerMethod("setStateByRequest", &MistBlock::setStateByRequest);
    MistBlock::registerMethod("set_state_by_request", &MistBlock::setStateByRequest);

    MistBlock::registerMethod("setStateByRequestUrlencoded", &MistBlock::setStateByRequestUrlencoded);
    MistBlock::registerMethod("set_state_by_request_urlencoded", &MistBlock::setStateByRequestUrlencoded);

    MistBlock::registerMethod("setStateByHeaders", &MistBlock::setStateByHeaders);
    MistBlock::registerMethod("set_state_by_headers", &MistBlock::setStateByHeaders);

    MistBlock::registerMethod("setStateByCookies", &MistBlock::setStateByCookies);
    MistBlock::registerMethod("set_state_by_cookies", &MistBlock::setStateByCookies);

    MistBlock::registerMethod("setStateByProtocol", &MistBlock::setStateByProtocol);
    MistBlock::registerMethod("set_state_by_protocol", &MistBlock::setStateByProtocol);

    MistBlock::registerMethod("echoQuery", &MistBlock::echoQuery);
    MistBlock::registerMethod("echo_query", &MistBlock::echoQuery);

    MistBlock::registerMethod("echoRequest", &MistBlock::echoRequest);
    MistBlock::registerMethod("echo_request", &MistBlock::echoRequest);

    MistBlock::registerMethod("echoHeaders", &MistBlock::echoHeaders);
    MistBlock::registerMethod("echo_headers", &MistBlock::echoHeaders);

    MistBlock::registerMethod("echoCookies", &MistBlock::echoCookies);
    MistBlock::registerMethod("echo_cookies", &MistBlock::echoCookies);

    MistBlock::registerMethod("echoProtocol", &MistBlock::echoProtocol);
    MistBlock::registerMethod("echo_protocol", &MistBlock::echoProtocol);

    MistBlock::registerMethod("setStateJoinString", &MistBlock::setStateJoinString);
    MistBlock::registerMethod("set_state_join_string", &MistBlock::setStateJoinString);

    MistBlock::registerMethod("setStateSplitString", &MistBlock::setStateSplitString);
    MistBlock::registerMethod("set_state_split_string", &MistBlock::setStateSplitString);

    MistBlock::registerMethod("setStateConcatString", &MistBlock::setStateConcatString);
    MistBlock::registerMethod("set_state_concat_string", &MistBlock::setStateConcatString);

    MistBlock::registerMethod("dropState", &MistBlock::dropState);
    MistBlock::registerMethod("drop_state", &MistBlock::dropState);

    MistBlock::registerMethod("dumpState", &MistBlock::dumpState);
    MistBlock::registerMethod("dump_state", &MistBlock::dumpState);

    MistBlock::registerMethod("attachStylesheet", &MistBlock::attachStylesheet);
    MistBlock::registerMethod("attach_stylesheet", &MistBlock::attachStylesheet);

    MistBlock::registerMethod("location", &MistBlock::location);

    MistBlock::registerMethod("setStatus", &MistBlock::setStatus);
    MistBlock::registerMethod("set_status", &MistBlock::setStatus);
}

static MistMethodRegistrator reg_;
static ExtensionRegisterer ext_(ExtensionHolder(new MistExtension()));

} // namespace xscript


extern "C" ExtensionInfo* get_extension_info() {
    static ExtensionInfo info = { "mist", xscript::XmlUtils::XSCRIPT_NAMESPACE };
    return &info;
}

