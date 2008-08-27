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
#include <xscript/xpath_expr.h>

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
    

    /**
     * Taggable implementation.
     */
    std::string createTagKey(const Context *ctx) const;
    // Part of Taggable 
    virtual bool tagged() const;
    virtual void tagged(bool value);

    bool hasTagOverride() const {
        return !tag_override_.empty();
    }

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
    
    static void appendNodeValue(xmlNodePtr node, std::string &val);

protected:

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

    InvokeResult fakeResult() const;

    bool xpathNode(const xmlNodePtr node) const;
    bool paramNode(const xmlNodePtr node) const;
    bool tagOverrideNode(const xmlNodePtr node) const;

    /**
     * Override tagKey and cached flag in result if tag-override specified
     */
    void evalTagOverride(InvokeResult &res) const;

    /**
     * Parse <xpath> node and put it container
     */
    void parseXPathNode(std::vector<XPathExpr> &container, const xmlNodePtr node);
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
    /// Array of XPathExpr used to override default tag calculation.
    std::vector<XPathExpr> tag_override_;
};

} // namespace xscript

#endif // _XSCRIPT_BLOCK_H_
