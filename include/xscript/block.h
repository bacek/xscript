#ifndef _XSCRIPT_BLOCK_H_
#define _XSCRIPT_BLOCK_H_

#include <utility>
#include <map>
#include <list>
#include <vector>
#include <exception>
#include <boost/any.hpp>
#include <xscript/object.h>
#include <xscript/extension.h>
#include <xscript/invoke_result.h>
#include <xscript/taggable.h>

namespace xscript {

class Xml;
class ParamFactory;

class Block : public Object, public Taggable {
public:
    Block(const Extension *ext, Xml *owner, xmlNodePtr node);
    virtual ~Block();

    inline Xml* owner() const {
        return owner_;
    }

    inline xmlNodePtr node() const {
        return node_;
    }

    inline const std::string& id() const {
        return id_;
    }

    inline const std::string& guard() const {
        return guard_;
    }

    inline const std::string& method() const {
        return method_;
    }

    const char* name() const {
        return extension_->name();
    }

    virtual bool threaded() const;
    virtual void threaded(bool value);
    
    virtual bool tagged() const;
    virtual void tagged(bool value);

    inline bool stripRootElement() const {
        return strip_root_element_;
    }

    inline void stripRootElement(bool value) {
        strip_root_element_ = value;
    }

    inline const Param* param(unsigned int n) const {
        return params_.at(n);
    }

    const Param* param(const std::string &id, bool throw_error = true) const;

    inline const std::vector<Param*>& params() const {
        return params_;
    }

    virtual void parse();
    virtual std::string fullName(const std::string &name) const;

    virtual InvokeResult invoke(Context *ctx);

    virtual void invokeCheckThreaded(boost::shared_ptr<Context> ctx, unsigned int slot);
    virtual void applyStylesheet(Context *ctx, XmlDocHelper &doc);

    InvokeResult errorResult(const char *error) const;
    InvokeResult errorResult(xmlNodePtr error_node) const;

    Logger * log() const {
        return extension_->getLogger();
    }
    
    /**
     * Taggable implementation.
     */
    std::string createTagKey(const Context *ctx) const;

protected:
    class XPathExpr;

    virtual InvokeResult invokeInternal(Context *ctx);
    virtual void postParse();
    virtual InvokeResult call(Context *ctx, boost::any &a) throw (std::exception) = 0;
    virtual InvokeResult processResponse(Context *ctx, InvokeResult doc, boost::any &a);
    virtual void property(const char *name, const char *value);
    virtual void postCall(Context *ctx, const XmlDocHelper &doc, const boost::any &a);
    virtual void callInternal(boost::shared_ptr<Context> ctx, unsigned int slot);
    virtual void callInternalThreaded(boost::shared_ptr<Context> ctx, unsigned int slot);

    bool checkGuard(Context *ctx) const;
    void evalXPath(Context *ctx, const XmlDocHelper &doc) const;
    void appendNodeValue(xmlNodePtr node, std::string &val) const;

    InvokeResult fakeResult() const;

    bool xpathNode(const xmlNodePtr node) const;
    bool paramNode(const xmlNodePtr node) const;

    void parseXPathNode(const xmlNodePtr node);
    void parseParamNode(const xmlNodePtr node, ParamFactory *pf);

    inline const std::vector<XPathExpr>& xpath() const {
        return xpath_;
    }

    InvokeResult errorResult(const char *error, xmlNodePtr error_node) const;

private:
    const Extension *extension_;
    Xml *owner_;
    xmlNodePtr node_;
    std::vector<Param*> params_;
    std::vector<XPathExpr> xpath_;
    std::string id_, guard_, method_;
    bool is_guard_not_;
    bool strip_root_element_;

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

} // namespace xscript

#endif // _XSCRIPT_BLOCK_H_
