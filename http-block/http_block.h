#ifndef _XSCRIPT_HTTP_BLOCK_H_
#define _XSCRIPT_HTTP_BLOCK_H_

#include <iosfwd>

#include "xscript/block.h"
#include "xscript/extension.h"
#include "xscript/remote_tagged_block.h"

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

typedef XmlDocHelper (HttpBlock::*HttpMethod)(Context *ctx, InvokeContext *invoke_ctx) const;

#ifndef HAVE_HASHMAP
typedef std::map<std::string, HttpMethod> MethodMap;
#else
typedef details::hash_map<std::string, HttpMethod, details::StringHash> MethodMap;
#endif

// TODO: Why it is not virtual inherited?
class HttpBlock : public RemoteTaggedBlock {
public:
    HttpBlock(const Extension *ext, Xml *owner, xmlNodePtr node);
    virtual ~HttpBlock();

protected:
    virtual void postParse();
    virtual void property(const char *name, const char *value);
    virtual void retryCall(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const throw (std::exception);

    std::string getUrl(const ArgList *args, unsigned int first, unsigned int last) const;
    
    XmlDocHelper getHttp(Context *ctx, InvokeContext *invoke_ctx) const;
    XmlDocHelper postHttp(Context *ctx, InvokeContext *invoke_ctx) const;
    XmlDocHelper getByState(Context *ctx, InvokeContext *invoke_ctx) const;
    XmlDocHelper getByRequest(Context *ctx, InvokeContext *invoke_ctx) const;
    XmlDocHelper postByRequest(Context *ctx, InvokeContext *invoke_ctx) const;
    XmlDocHelper getBinaryPage(Context *ctx, InvokeContext *invoke_ctx) const;

    XmlDocHelper response(const HttpHelper &h, bool error_mode = false) const;
    void appendHeaders(HttpHelper &helper, const Request *request, InvokeContext *invoke_ctx) const;
    void createTagInfo(const HttpHelper &h, InvokeContext *invoke_ctx) const;

    static void registerMethod(const char *name, HttpMethod method);

private:
    HttpBlock(const HttpBlock &);
    HttpBlock& operator = (const HttpBlock &);
    
    int getTimeout(Context *ctx, const std::string &url) const;
    void wrapError(InvokeError &error, const HttpHelper &helper, const XmlNodeHelper &error_body_node) const;
    void checkStatus(const HttpHelper &helper) const;
    void httpCall(HttpHelper &helper) const;
    void createMeta(HttpHelper &helper, InvokeContext *invoke_ctx) const;

    friend class HttpMethodRegistrator;

private:
    bool proxy_;
    bool print_error_;
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
