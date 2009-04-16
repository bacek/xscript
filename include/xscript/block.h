#ifndef _XSCRIPT_BLOCK_H_
#define _XSCRIPT_BLOCK_H_

#include <map>
#include <list>
#include <vector>
#include <exception>
#include <boost/any.hpp>
#include <xscript/object.h>
#include <xscript/xml_helpers.h>
#include <xscript/extension.h>
#include <xscript/util.h>
#include <xscript/invoke_result.h>

namespace xscript {

class Xml;
class ParamFactory;

class Block : public Object {
public:
    Block(const Extension *ext, Xml *owner, xmlNodePtr node);
    virtual ~Block();

    Xml* owner() const;
    xmlNodePtr node() const;
    const std::string& id() const;
    const std::string& guard() const;
    const std::string& method() const;
    const std::string& xpointerExpr() const;
    const char* name() const;

    virtual bool threaded() const;
    virtual void threaded(bool value);

    virtual bool tagged() const;

    bool xpointer(Context* ctx) const;

    const Param* param(unsigned int n) const;
    const Param* param(const std::string &id, bool throw_error = true) const;
    const std::vector<Param*>& params() const;

    virtual void parse();
    virtual std::string fullName(const std::string &name) const;

    virtual InvokeResult invoke(boost::shared_ptr<Context> ctx);
    virtual void invokeCheckThreaded(boost::shared_ptr<Context> ctx, unsigned int slot);
    virtual void applyStylesheet(boost::shared_ptr<Context> ctx, XmlDocHelper &doc);

    InvokeResult errorResult(const char *error) const;
    InvokeResult errorResult(const char *error, const InvokeError::InfoMapType &error_info) const;
    InvokeResult errorResult(const char *error, const InvokeError::InfoMapType &error_info, std::string &full_error) const;
    std::string errorMessage(const char *error, const InvokeError::InfoMapType &error_info) const;

    void throwBadArityError() const;

    Logger * log() const;

    virtual void startTimer(Context *ctx);
    virtual void stopTimer(Context *ctx);
    
protected:
    class XPathExpr;

    virtual InvokeResult invokeInternal(boost::shared_ptr<Context> ctx);
    virtual void postParse();
    virtual XmlDocHelper call(boost::shared_ptr<Context> ctx, boost::any &a) throw (std::exception) = 0;
    virtual XmlDocHelper processResponse(boost::shared_ptr<Context> ctx, XmlDocHelper doc, boost::any &a);
    virtual void property(const char *name, const char *value);
    virtual void postCall(Context *ctx, const XmlDocHelper &doc, const boost::any &a);
    virtual void postInvoke(Context *ctx, const XmlDocHelper &doc);
    virtual void callInternal(boost::shared_ptr<Context> ctx, unsigned int slot);
    virtual void callInternalThreaded(boost::shared_ptr<Context> ctx, unsigned int slot);

    bool checkGuard(Context *ctx) const;
    void evalXPath(Context *ctx, const XmlDocHelper &doc) const;
    void appendNodeValue(xmlNodePtr node, std::string &val) const;

    XmlDocHelper fakeResult() const;

    bool xpathNode(const xmlNodePtr node) const;
    bool paramNode(const xmlNodePtr node) const;

    void parseXPathNode(const xmlNodePtr node);
    void parseParamNode(const xmlNodePtr node, ParamFactory *pf);
    virtual void processParam(std::auto_ptr<Param> p);

    const std::vector<XPathExpr>& xpath() const;
    
    virtual std::string concatParams(const Context *ctx, unsigned int begin, unsigned int end) const; 
    
private:
    struct BlockData;
    BlockData *data_;

protected:
    class XPathExpr {
    public:
        XPathExpr(const char* expression, const char* result, const char* delimeter) :
                expression_(expression ? expression : ""),
                result_(result ? result : ""),
                delimeter_(delimeter ? delimeter : "") {}
        ~XPathExpr() {}

        typedef std::list<std::pair<std::string, std::string> > NamespaceListType;

        const std::string& expression() const {
            return expression_;
        }

        const std::string& result() const {
            return result_;
        }

        const std::string& delimeter() const {
            return delimeter_;
        }

        const NamespaceListType& namespaces() const {
            return namespaces_;
        }

        void addNamespace(const char* prefix, const char* uri) {
            if (prefix && uri) {
                namespaces_.push_back(std::make_pair(std::string(prefix), std::string(uri)));
            }
        }

    private:
        std::string expression_;
        std::string result_;
        std::string delimeter_;
        NamespaceListType namespaces_;
    };
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
