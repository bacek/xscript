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

class ArgList;
class Context;
class MetaBlock;
class ParamFactory;
class Xml;

class InvokeHelper {
public:
    InvokeHelper(boost::shared_ptr<Context> ctx);
    ~InvokeHelper();

    const boost::shared_ptr<Context>& context() const;
private:
    std::list<boost::shared_ptr<Context> > parents_;
    boost::shared_ptr<Context> ctx_;
};

class Block : public Object {
public:
    Block(const Extension *ext, Xml *owner, xmlNodePtr node);
    virtual ~Block();

    Xml* owner() const;
    xmlNodePtr node() const;
    const std::string& id() const;
    const std::string& method() const;
    const char* name() const;

    virtual bool threaded() const;
    virtual void threaded(bool value);

    virtual bool tagged() const;

    const Param* param(unsigned int n) const;
    const Param* param(const std::string &id, bool throw_error = true) const;
    const std::vector<Param*>& params() const;

    virtual void parse();
    virtual std::string fullName(const std::string &name) const;

    virtual void call(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const throw (std::exception) = 0;
    virtual boost::shared_ptr<InvokeContext> invoke(boost::shared_ptr<Context> ctx); //TODO: remove virtual
    virtual void invokeCheckThreaded(boost::shared_ptr<Context> ctx, unsigned int slot); //TODO: remove this

    bool invokeCheckThreadedEx(boost::shared_ptr<Context> ctx, unsigned int slot);
    virtual bool applyStylesheet(boost::shared_ptr<Context> ctx, XmlDocSharedHelper &doc);

    boost::shared_ptr<InvokeContext> errorResult(const char *error, bool info) const;
    boost::shared_ptr<InvokeContext> errorResult(const InvokeError &error, bool info) const;
    boost::shared_ptr<InvokeContext> fakeResult(bool error) const;

    void throwBadArityError() const;

    Logger * log() const;

    virtual void startTimer(Context *ctx);
    virtual void stopTimer(Context *ctx);

    typedef xmlNodePtr (*insertFunc)(xmlNodePtr, xmlNodePtr);
    xmlNodePtr processXPointer(const InvokeContext *invoke_ctx, xmlDocPtr doc, xmlDocPtr meta_doc,
            xmlNodePtr insert_node, insertFunc func) const;
    
    const Extension* extension() const;
    MetaBlock* metaBlock() const;
    bool checkGuard(Context *ctx) const;
    const std::map<std::string, std::string>& namespaces() const;

protected:
    boost::shared_ptr<InvokeContext> createInvokeContext(boost::shared_ptr<Context> ctx) const;
    boost::shared_ptr<InvokeContext> invoke_Ex(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx);

    virtual void invokeInternal(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx);
    virtual void postParse();
    virtual void processResponse(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx);
    virtual void property(const char *name, const char *value);
    virtual void postCall(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx);
    virtual void postInvoke(Context *ctx, InvokeContext *invoke_ctx);

    virtual void callInternal(boost::shared_ptr<Context> ctx, unsigned int slot); //TODO: remove this
    void callInternal_Ex(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx, unsigned int slot);

    virtual void callInternalThreaded(boost::shared_ptr<Context> ctx, unsigned int slot); //TODO: remove this
    void callInternalThreaded_Ex(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx, unsigned int slot);
    void callInternalThreadedHelper(InvokeHelper helper, boost::shared_ptr<InvokeContext> invoke_ctx, unsigned int slot);

    virtual void callMetaLua(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx);
    virtual ArgList* createArgList(Context *ctx, InvokeContext *invoke_ctx) const;
    void processArguments(Context *ctx, InvokeContext *invoke_ctx) const;
    void callMeta(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx);
    bool checkStateGuard(Context *ctx) const;
    bool hasGuard() const;
    bool hasStateGuard() const;
    void evalXPath(Context *ctx, InvokeContext *invoke_ctx, const XmlDocSharedHelper &doc) const;
    bool applyStylesheet(boost::shared_ptr<Context> ctx,
        boost::shared_ptr<InvokeContext> invoke_ctx, XmlDocSharedHelper &doc);

    void appendNodeValue(xmlNodePtr node, std::string &val) const;

    virtual bool xpathNode(const xmlNodePtr node) const;
    virtual bool guardNode(const xmlNodePtr node) const;
    virtual bool guardNotNode(const xmlNodePtr node) const;
    virtual bool paramNode(const xmlNodePtr node) const;
    bool xpointerNode(const xmlNodePtr node) const;
    bool metaNode(const xmlNodePtr node) const;
    bool xsltNode(const xmlNodePtr node) const;

    virtual void parseSubNode(xmlNodePtr node);
    virtual void parseXPathNode(const xmlNodePtr node);
    virtual void parseGuardNode(const xmlNodePtr node, bool is_not);
    virtual void parseParamNode(const xmlNodePtr node);
    void parseXsltNode(const xmlNodePtr node);
    void parseXPointerNode(const xmlNodePtr node);
    void parseMetaNode(const xmlNodePtr node);
    void parseXPointerExpr(const char *value, const char *type);
    void disableOutput(bool flag);
    bool disableOutput() const;
    xmlNodePtr processEmptyXPointer(const InvokeContext *invoke_ctx, xmlDocPtr meta_doc,
            xmlNodePtr insert_node, insertFunc func) const;
    
    static std::string concatArguments(const ArgList *args, unsigned int first, unsigned int last);

    void addParam(std::auto_ptr<Param> param);
    void detectBase();
    
    void addNamespaces(const std::map<std::string, std::string> &ns);

    bool isMeta() const;

private:
    std::string errorLocation() const;
    std::string errorMessage(const InvokeError &error) const;
    boost::shared_ptr<InvokeContext> errorResult(XmlDocHelper doc) const;
    boost::shared_ptr<InvokeContext> errorResult(const InvokeError &error, std::string &full_error) const;
    void errorResult(const char *error, bool info, boost::shared_ptr<InvokeContext> &ctx) const;
    void metaErrorResult(const MetaInvokeError &error, boost::shared_ptr<InvokeContext> &invoke_ctx) const;
    XmlDocHelper errorDoc(const InvokeError &error, const char *tag_name, std::string &full_error) const;
    XmlDocHelper fakeDoc() const;
    
private:
    struct BlockData;
    std::auto_ptr<BlockData> data_;
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
