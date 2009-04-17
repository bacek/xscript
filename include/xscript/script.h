#ifndef _XSCRIPT_SCRIPT_H_
#define _XSCRIPT_SCRIPT_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/thread/mutex.hpp>

#include <xscript/functors.h>
#include <xscript/object.h>
#include <xscript/xml.h>
#include <xscript/xml_helpers.h>

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
class Script : public virtual Object, public Xml {
public:
    virtual ~Script();

    inline bool threaded() const {
        return flags_ & FLAG_THREADED;
    }

    inline bool forceStylesheet() const {
        return flags_ & FLAG_FORCE_STYLESHEET;
    }

    inline bool binaryPage() const {
        return flags_ & FLAG_BINARY_PAGE;
    }

    inline unsigned int expireTimeDelta() const {
        return expire_time_delta_;
    }

    inline time_t cacheTime() const {
        return cache_time_;
    }
    
    bool allowMethod(const std::string& value) const;

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

    virtual const std::string& name() const;

    virtual std::string fullName(const std::string &name) const;

    virtual void removeUnusedNodes(const XmlDocHelper &helper);
    virtual XmlDocHelper invoke(boost::shared_ptr<Context> ctx);
    virtual void applyStylesheet(boost::shared_ptr<Context> ctx, XmlDocHelper &doc);

    virtual std::string createTagKey(const Context *ctx) const;
    virtual std::string info(const Context *ctx) const;
    virtual bool cachable(const Context *ctx, bool for_save) const;
    
    void addExpiresHeader(const Context *ctx) const;

    const std::string& extensionProperty(const std::string &name) const;
    
    // TODO: remove this method. It should be in ScriptFactory.
    static boost::shared_ptr<Script> create(const std::string &name);
    static boost::shared_ptr<Script> create(const std::string &name, const std::string &xml);

protected:
    Script(const std::string &name);

    inline void threaded(bool value) {
        flag(FLAG_THREADED, value);
    }

    inline void forceStylesheet(bool value) {
        flag(FLAG_FORCE_STYLESHEET, value);
    }

    inline void expireTimeDelta(unsigned int value) {
        expire_time_delta_ = value;
    }

    inline void cacheTime(time_t value) {
        cache_time_ = value;
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
    void useXpointerExpr(xmlDocPtr doc, xmlNodePtr newnode, xmlChar *xpath) const;

    int countTimeout() const;
    void addHeaders(Context *ctx) const;

    inline const std::vector<Block*>& blocks() const {
        return blocks_;
    }

    XmlDocHelper fetchResults(Context *ctx) const;
    void fetchRecursive(Context *ctx, xmlNodePtr node, xmlNodePtr newnode, unsigned int &count, unsigned int &xscript_count) const;

    virtual void parse();
    virtual void parse(const std::string &xml);
    virtual void postParse();
    virtual void property(const char *name, const char *value);

    bool cacheTimeUndefined() const;
    
    virtual void replaceXScriptNode(xmlNodePtr node, xmlNodePtr newnode, Context *ctx) const;
    
    static boost::shared_ptr<Script> createWithParse(const std::string &name, const std::string &xml);

    static const unsigned int FLAG_THREADED = 1;
    static const unsigned int FLAG_FORCE_STYLESHEET = 1 << 1;
    static const unsigned int FLAG_BINARY_PAGE = 1 << 2;
    
    static const std::string GET_METHOD;
private:
    friend class ScriptFactory;

private:
    XmlDocHelper doc_;
    std::vector<Block*> blocks_;
    std::string name_;
    unsigned int flags_;
    unsigned int expire_time_delta_;
    time_t cache_time_;
    std::set<xmlNodePtr> xscript_node_set_;
    std::map<std::string, std::string> headers_;
    std::vector<std::string> allow_methods_;
    std::map<std::string, std::string, StringCILess> extension_properties_;
};

} // namespace xscript

#endif // _XSCRIPT_SCRIPT_H_
