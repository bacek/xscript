#ifndef _XSCRIPT_HTTP_BLOCK_H_
#define _XSCRIPT_HTTP_BLOCK_H_

#include "settings.h"

#include <iosfwd>

#include "xscript/block.h"
#include "xscript/extension.h"
#include "xscript/threaded_tagged_block.h"

#include "internal/hash.h"
#include "internal/hashmap.h"

#ifndef HAVE_HASHMAP
#include <map>
#endif

namespace xscript {

class Loader;
class Context;

class HttpBlock;
class HttpHelper;

class HttpMethodRegistrator;

typedef InvokeResult (HttpBlock::*HttpMethod)(Context *ctx, boost::any &);

#ifndef HAVE_HASHMAP
typedef std::map<std::string, HttpMethod> MethodMap;
#else
typedef details::hash_map<std::string, HttpMethod, details::StringHash> MethodMap;
#endif

// TODO: Why it is not virtual inherited?
class HttpBlock : public ThreadedTaggedBlock {
public:
    HttpBlock(const Extension *ext, Xml *owner, xmlNodePtr node);
    virtual ~HttpBlock();

protected:
    virtual void postParse();
    virtual void property(const char *name, const char *value);
    virtual InvokeResult call(Context *ctx, boost::any &a) throw (std::exception);

    InvokeResult getHttp(Context *ctx, boost::any &a);

    // Proxy for getHttp with emiting notice about obsoleted call.
    InvokeResult getHttpObsolete(Context *ctx, boost::any &a);

    InvokeResult postHttp(Context *ctx, boost::any &a);
    InvokeResult getByState(Context *ctx, boost::any &a);
    InvokeResult getByRequest(Context *ctx, boost::any &a);

    InvokeResult response(const HttpHelper &h) const;
    void createTagInfo(const HttpHelper &h, boost::any &a) const;

    static void registerMethod(const char *name, HttpMethod method);

private:
    HttpBlock(const HttpBlock &);
    HttpBlock& operator = (const HttpBlock &);

    friend class HttpMethodRegistrator;

private:
    bool proxy_;
    HttpMethod method_;
    std::string charset_;
    static MethodMap methods_;
};

class HttpExtension : public Extension {
public:
    HttpExtension();
    virtual ~HttpExtension();

    virtual const char* name() const;
    virtual const char* nsref() const;

    virtual void initContext(Context *ctx);
    virtual void stopContext(Context *ctx);
    virtual void destroyContext(Context *ctx);

    virtual std::auto_ptr<Block> createBlock(Xml *owner, xmlNodePtr node);
    virtual void init(const Config *config);

private:
    HttpExtension(const HttpExtension &);
    HttpExtension& operator = (const HttpExtension &);
};

} // namespace xscript

#endif // _XSCRIPT_HTTP_BLOCK_H_
