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

    bool threaded() const;
    bool forceStylesheet() const;
    bool binaryPage() const;
    unsigned int expireTimeDelta() const;
    time_t cacheTime() const;
    
    bool allowMethod(const std::string& value) const;
    
    const Block* block(unsigned int n) const;
    const Block* block(const std::string &id, bool throw_error = true) const;
    unsigned int blocksNumber() const;

    const std::string& header(const std::string &name) const;
    const std::map<std::string, std::string>& headers() const;

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

    void threaded(bool value);
    void forceStylesheet(bool value);
    void expireTimeDelta(unsigned int value);
    void cacheTime(time_t value);
    void binaryPage(bool value);
    void flag(unsigned int type, bool value);

    void allowMethods(const char *value);
    void extensionProperty(const char *prop, const char *value);

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

    const std::vector<Block*>& blocks() const;

    XmlDocHelper fetchResults(Context *ctx) const;
    void fetchRecursive(Context *ctx, xmlNodePtr node, xmlNodePtr newnode, unsigned int &count, unsigned int &xscript_count) const;

    virtual void parse();
    virtual void parse(const std::string &xml);
    virtual void postParse();
    virtual void property(const char *name, const char *value);

    bool cacheTimeUndefined() const;
    
    virtual void replaceXScriptNode(xmlNodePtr node, xmlNodePtr newnode, Context *ctx) const;
    
    static boost::shared_ptr<Script> createWithParse(const std::string &name, const std::string &xml);

private:
    friend class ScriptFactory;
    
    class ScriptData;
    ScriptData *data_;
    
    static const std::string GET_METHOD;
};

} // namespace xscript

#endif // _XSCRIPT_SCRIPT_H_
