#ifndef _XSCRIPT_BLOCK_H_
#define _XSCRIPT_BLOCK_H_

#include <exception>
#include <list>
#include <map>
#include <vector>

#include <xscript/exception.h>
#include <xscript/extension.h>
#include "xscript/invoke_context.h"
#include <xscript/object.h>
#include <xscript/xml_helpers.h>

namespace xscript {

class Context;
class Xml;
class ParamFactory;

class Block : public Object {
public:
    Block(const Extension *ext, Xml *owner, xmlNodePtr node);
    virtual ~Block();

    Xml* owner() const;
    xmlNodePtr node() const;
    const std::string& id() const;
    const std::string& method() const;
    const std::string& xpointerExpr() const;
    const char* name() const;

    virtual bool threaded() const;
    virtual void threaded(bool value);

    virtual bool tagged() const;
    bool disableOutput() const;
    
    bool xpointer(const Context* ctx) const;
    XmlXPathObjectHelper evalXPointer(xmlDocPtr doc) const;

    const Param* param(unsigned int n) const;
    const Param* param(const std::string &id, bool throw_error = true) const;
    const std::vector<Param*>& params() const;

    virtual void parse();
    virtual std::string fullName(const std::string &name) const;

    virtual boost::shared_ptr<InvokeContext> invoke(boost::shared_ptr<Context> ctx);
    virtual void invokeCheckThreaded(boost::shared_ptr<Context> ctx, unsigned int slot);
    virtual bool applyStylesheet(boost::shared_ptr<Context> ctx, XmlDocSharedHelper &doc);

    boost::shared_ptr<InvokeContext> errorResult(const char *error, bool info) const;
    boost::shared_ptr<InvokeContext> errorResult(const InvokeError &error, bool info) const;
    boost::shared_ptr<InvokeContext> fakeResult(bool error) const;
       
    void throwBadArityError() const;

    Logger * log() const;

    virtual void startTimer(Context *ctx);
    virtual void stopTimer(Context *ctx);
    
protected:
    virtual void invokeInternal(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx);
    virtual void postParse();
    virtual XmlDocHelper call(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) throw (std::exception) = 0;
    virtual void processResponse(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx);
    virtual void property(const char *name, const char *value);
    virtual void postCall(Context *ctx, InvokeContext *invoke_ctx);
    virtual void postInvoke(Context *ctx, InvokeContext *invoke_ctx);
    virtual void callInternal(boost::shared_ptr<Context> ctx, unsigned int slot);
    virtual void callInternalThreaded(boost::shared_ptr<Context> ctx, unsigned int slot);
    
    bool checkGuard(Context *ctx) const;
    void evalXPath(Context *ctx, const XmlDocSharedHelper &doc) const;
    void appendNodeValue(xmlNodePtr node, std::string &val) const;

    virtual bool xpathNode(const xmlNodePtr node) const;
    virtual bool guardNode(const xmlNodePtr node) const;
    virtual bool guardNotNode(const xmlNodePtr node) const;
    virtual bool paramNode(const xmlNodePtr node) const;

    virtual void parseSubNode(xmlNodePtr node);
    virtual void parseXPathNode(const xmlNodePtr node);
    virtual void parseGuardNode(const xmlNodePtr node, bool is_not);
    virtual void parseParamNode(const xmlNodePtr node);
    
    virtual std::string concatParams(const Context *ctx, unsigned int begin, unsigned int end) const;
    
    void addParam(std::auto_ptr<Param> param);
    void detectBase();
    
private:
    std::string errorMessage(const InvokeError &error) const;
    boost::shared_ptr<InvokeContext> errorResult(XmlDocHelper doc) const;
    boost::shared_ptr<InvokeContext> errorResult(const InvokeError &error, std::string &full_error) const;
    void errorResult(const char *error, bool info, boost::shared_ptr<InvokeContext> &ctx) const;
    XmlDocHelper errorDoc(const InvokeError &error, const char *tag_name, std::string &full_error) const;
    XmlDocHelper fakeDoc() const;
    
private:
    struct BlockData;
    friend struct BlockData;
    BlockData *data_;
};

struct BlockTimerStarter {
    BlockTimerStarter(Context *ctx, Block *block) : ctx_(ctx), block_(block) {
        block_->startTimer(ctx_);
    }
    virtual ~BlockTimerStarter() {
        block_->stopTimer(ctx_);
    }
private:
    Context *ctx_;
    Block *block_;
};

} // namespace xscript

#endif // _XSCRIPT_BLOCK_H_
