#ifndef _XSCRIPT_SCRIPT_H_
#define _XSCRIPT_SCRIPT_H_

#include <map>
#include <set>
#include <string>
#include <vector>
#include <xscript/xml.h>
#include <xscript/object.h>
#include <xscript/xml_helpers.h>
#include <xscript/invoke_result.h>
#include <xscript/taggable.h>

namespace xscript {

class Block;
class Context;
class ScriptFactory;

/**
 * Parsed script.
 *
 * Stores parse blocks, assosiated stylesheet and various processing flags.
 * Created by ScriptFactory (for caching purposes).
 */
class Script : public virtual Object, public Xml, public Taggable {
public:
    virtual ~Script();

    inline bool threaded() const {
        return flags_ & FLAG_THREADED;
    }

    inline bool needAuth() const {
        return flags_ & FLAG_NEED_AUTH;
    }

    inline bool forceAuth() const {
        return flags_ & FLAG_FORCE_AUTH;
    }

    inline bool forceStylesheet() const {
        return flags_ & FLAG_FORCE_STYLESHEET;
    }

    inline bool binaryPage() const {
        return flags_ & FLAG_BINARY_PAGE;
    }

    inline bool cacheWholePage() const {
        return flags_ & FLAG_CACHE_WHOLE_PAGE;
    }

    inline unsigned int expireTimeDelta() const {
        return expire_time_delta_;
    }

    bool allowMethod(const std::string& value) const;

    inline void needAuth(bool value) {
        flag(FLAG_NEED_AUTH, value);
    }

    inline const Block* block(unsigned int n) const {
        return blocks_.at(n);
    }

    const Block* block(const std::string &id, bool throw_error = true) const;

    unsigned int blocksNumber() const {
        return blocks_.size();
    }

    const std::string& header(const std::string &name) const;

    inline const std::map<std::string, std::string>& headers() const {
        return headers_;
    }

    virtual time_t modified() const;
    virtual const std::string& name() const;

    virtual std::string fullName(const std::string &name) const;

    virtual void removeUnusedNodes(const XmlDocHelper &helper);
    // Invoke script.
    // TODO Do not construct result tree until required.
    virtual InvokeResult invoke(boost::shared_ptr<Context> ctx);
    virtual void applyStylesheet(Context *ctx, XmlDocHelper &doc);

    // TODO: remove this method. It should be in ScriptFactory.
    static boost::shared_ptr<Script> create(const std::string &name);

    // Taggable implementation
    virtual std::string createTagKey(const Context *ctx) const;
protected:
    Script(const std::string &name);

    inline void threaded(bool value) {
        flag(FLAG_THREADED, value);
    }

    inline void forceAuth(bool value) {
        if (value) {
            needAuth(true);
        }
        flag(FLAG_FORCE_AUTH, value);
    }

    inline void forceStylesheet(bool value) {
        flag(FLAG_FORCE_STYLESHEET, value);
    }

    inline void expireTimeDelta(unsigned int value) {
        expire_time_delta_ = value;
    }

    inline void binaryPage(bool value) {
        flag(FLAG_BINARY_PAGE, value);
    }

    inline void flag(unsigned int type, bool value) {
        flags_ = value ? (flags_ | type) : (flags_ &= ~type);
    }

    void allowMethods(const char *value);

    void parseNode(xmlNodePtr node, std::vector<xmlNodePtr>& xscript_nodes);
    void parseHeadersNode(xmlNodePtr node);
    virtual void parseXScriptNode(const xmlNodePtr node);
    void parseXScriptNodes(std::vector<xmlNodePtr>& xscript_nodes);
    void parseBlocks();
    void buildXScriptNodeSet(std::vector<xmlNodePtr>& xscript_nodes);
    void parseStylesheetNode(const xmlNodePtr node);

    int countTimeout() const;
    void addHeaders(Context *ctx) const;

    inline const std::vector<Block*>& blocks() const {
        return blocks_;
    }

    // Construct result tree.
    // TODO Move to public API to allow constructing tree on demand.
    InvokeResult fetchResults(Context *ctx) const;

    // Fetch all result nodes from block and place them into result tree.
    // Returns "true" if all blocks responded with cached result
    bool fetchRecursive(Context *ctx, xmlNodePtr node, xmlNodePtr newnode, unsigned int &count, unsigned int &xscript_count) const;

    virtual void parse();
    virtual void postParse();
    virtual void property(const char *name, const char *value);

    virtual void replaceXScriptNode(xmlNodePtr node, xmlNodePtr newnode, Context *ctx) const;

    void removeUnusedNode(const XmlDocHelper &doc, xmlNodePtr orig);
    bool removeUnusedNode(xmlNodePtr node, xmlNodePtr newnode, xmlNodePtr orig);

    static const unsigned int FLAG_THREADED = 1;
    static const unsigned int FLAG_FORCE_STYLESHEET = 1 << 1;

    static const unsigned int FLAG_NEED_AUTH = 1 << 2;
    static const unsigned int FLAG_FORCE_AUTH = 1 << 3;
    static const unsigned int FLAG_BINARY_PAGE = 1 << 4;

    // Cache whole page html.
    static const unsigned int FLAG_CACHE_WHOLE_PAGE = 1 << 5;
private:
    friend class ScriptFactory;

private:
    time_t modified_;
    XmlDocHelper doc_;
    std::vector<Block*> blocks_;
    std::string name_;
    unsigned int flags_, expire_time_delta_;
    xmlNodePtr stylesheet_node_;
    std::set<xmlNodePtr> xscript_node_set_;
    std::map<std::string, std::string> headers_;
    std::vector<std::string> allow_methods_;
};

} // namespace xscript

#endif // _XSCRIPT_SCRIPT_H_
