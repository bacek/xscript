#ifndef _XSCRIPT_MIST_BLOCK_H_
#define _XSCRIPT_MIST_BLOCK_H_

#include "xscript/block.h"
#include "xscript/extension.h"

#include "internal/hash.h"
#include "internal/hashmap.h"

#ifndef HAVE_HASHMAP
#include <map>
#endif

namespace xscript {

class MistBlock;
class MistMethodRegistrator;

typedef xmlNodePtr (MistBlock::*MistMethod)(Context*);

#ifndef HAVE_HASHMAP
typedef std::map<std::string, MistMethod> MethodMap;
#else
typedef details::hash_map<std::string, MistMethod, details::StringHash> MethodMap;
#endif

class MistBlock : public Block {
public:
    MistBlock(const Extension *ext, Xml *owner, xmlNodePtr node);
    virtual ~MistBlock();

protected:
    virtual void postParse();
    virtual XmlDocHelper call(Context *ctx, boost::any &a) throw (std::exception);

protected:
    xmlNodePtr setStateLong(Context *ctx);
    xmlNodePtr setStateString(Context *ctx);
    xmlNodePtr setStateDouble(Context *ctx);
    xmlNodePtr setStateLongLong(Context *ctx);

    xmlNodePtr setStateRandom(Context *ctx);
    xmlNodePtr setStateDefined(Context *ctx);

    xmlNodePtr setStateUrlencode(Context *ctx);
    xmlNodePtr setStateUrldecode(Context *ctx);
    xmlNodePtr setStateXmlescape(Context *ctx);
    
    xmlNodePtr setStateDomain(Context *ctx);

    xmlNodePtr setStateByKeys(Context *ctx);
    xmlNodePtr setStateByDate(Context *ctx);

    xmlNodePtr setStateByQuery(Context *ctx);
    xmlNodePtr setStateByRequest(Context *ctx);
    xmlNodePtr setStateByRequestUrlencoded(Context *ctx);

    xmlNodePtr setStateByHeaders(Context *ctx);
    xmlNodePtr setStateByCookies(Context *ctx);
    xmlNodePtr setStateByProtocol(Context *ctx);

    xmlNodePtr echoQuery(Context *ctx);
    xmlNodePtr echoRequest(Context *ctx);
    xmlNodePtr echoHeaders(Context *ctx);
    xmlNodePtr echoCookies(Context *ctx);
    xmlNodePtr echoProtocol(Context *ctx);

    xmlNodePtr setStateJoinString(Context *ctx);
    xmlNodePtr setStateSplitString(Context *ctx);
    xmlNodePtr setStateConcatString(Context *ctx);

    xmlNodePtr dropState(Context *ctx);
    xmlNodePtr dumpState(Context *ctx);
    xmlNodePtr attachStylesheet(Context *ctx);
    xmlNodePtr location(Context *ctx);
    xmlNodePtr setStatus(Context *ctx);

protected:
    xmlNodePtr setStateList(Context *ctx, const char *name);
    static void registerMethod(const char* name, MistMethod method);

private:
    MistBlock(const MistBlock &);
    MistBlock& operator = (const MistBlock &);

    friend class MistMethodRegistrator;

private:
    MistMethod method_;
    static MethodMap methods_;
};

class MistExtension : public Extension {
public:
    MistExtension();
    virtual ~MistExtension();

    virtual const char* name() const;
    virtual const char* nsref() const;

    virtual void initContext(Context *ctx);
    virtual void stopContext(Context *ctx);
    virtual void destroyContext(Context *ctx);

    virtual std::auto_ptr<Block> createBlock(Xml *owner, xmlNodePtr node);
    virtual void init(const Config *config);

private:
    MistExtension(const MistExtension &);
    MistExtension& operator = (const MistExtension &);
};

} // namespace xscript

#endif // _XSCRIPT_MIST_BLOCK_H_
