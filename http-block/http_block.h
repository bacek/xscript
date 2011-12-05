#ifndef _XSCRIPT_HTTP_BLOCK_H_
#define _XSCRIPT_HTTP_BLOCK_H_

#include <string>
#include <set>
#include <vector>

#include "xscript/functors.h"
#include "xscript/remote_tagged_block.h"
#include "query_params.h"

namespace xscript {

class Loader;
class Context;
class Param;

class HttpBlock;
class HttpHelper;

class HttpMethodRegistrator;

typedef XmlDocHelper (HttpBlock::*HttpMethod)(Context *ctx, InvokeContext *invoke_ctx) const;

// TODO: Why it is not virtual inherited?
class HttpBlock : public RemoteTaggedBlock {
public:
    HttpBlock(const Extension *ext, Xml *owner, xmlNodePtr node);
    virtual ~HttpBlock();

protected:
    virtual void parseSubNode(xmlNodePtr node);
    virtual void postParse();
    virtual void property(const char *name, const char *value);
    virtual void retryCall(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const throw (std::exception);

    virtual std::string info(const Context *ctx) const;
    virtual std::string createTagKey(const Context *ctx, const InvokeContext *invoke_ctx) const;
    virtual ArgList* createArgList(Context *ctx, InvokeContext *invoke_ctx) const;

    std::string getUrl(const ArgList *args, unsigned int first, unsigned int last) const;
    std::string queryParams(const InvokeContext *invoke_ctx) const;
    bool createPostData(const Context *ctx, const InvokeContext *invoke_ctx, std::string &result) const;
    
    XmlDocHelper getHttp(Context *ctx, InvokeContext *invoke_ctx) const;
    XmlDocHelper post(Context *ctx, InvokeContext *invoke_ctx) const;
    XmlDocHelper postHttp(Context *ctx, InvokeContext *invoke_ctx) const;
    XmlDocHelper getByState(Context *ctx, InvokeContext *invoke_ctx) const;
    XmlDocHelper getByRequest(Context *ctx, InvokeContext *invoke_ctx) const;
    XmlDocHelper postByRequest(Context *ctx, InvokeContext *invoke_ctx) const;
    XmlDocHelper getBinaryPage(Context *ctx, InvokeContext *invoke_ctx) const;

    XmlDocHelper response(const HttpHelper &h, bool error_mode = false) const;
    void appendHeaders(HttpHelper &helper, const Request *request, const InvokeContext *invoke_ctx,
            bool allow_tag, bool pass_ctype) const;
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

    static void checkHeaderParamId(const std::string &repr_name, const std::string &id);
    static void checkQueryParamId(const std::string &repr_name, const std::string &id);

    friend class HttpMethodRegistrator;

private:
    QueryParams query_params_;
    std::vector<Param*> headers_;
    std::set<std::string, StringCILess> header_names_;
    std::string charset_;
    HttpMethod method_;
    bool proxy_;
    bool xff_;
    bool print_error_;
};

} // namespace xscript

#endif // _XSCRIPT_HTTP_BLOCK_H_
