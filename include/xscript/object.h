#ifndef _XSCRIPT_OBJECT_H_
#define _XSCRIPT_OBJECT_H_

#include <string>
#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include <xscript/xml_helpers.h>

namespace xscript {

class Block;
class Param;
class Context;
class Stylesheet;
class ParamFactory;

class Object : private boost::noncopyable {
public:
    Object();
    virtual ~Object();

    const std::string& xsltName() const;
    const std::vector<Param*>& xsltParams() const;

    virtual std::string fullName(const std::string &name) const = 0;
    virtual void applyStylesheet(boost::shared_ptr<Context> ctx, XmlDocHelper &doc) = 0;

    virtual std::string createTagKey(const Context *ctx) const;
    
protected:
    virtual void postParse();
    
    void xsltName(const std::string &value);
    bool xsltParamNode(const xmlNodePtr node) const;

    void parseXsltParamNode(const xmlNodePtr node);
    void applyStylesheet(boost::shared_ptr<Stylesheet> sh, boost::shared_ptr<Context> ctx, XmlDocHelper &doc, bool need_copy);
    
    std::auto_ptr<Param> createParam(const xmlNodePtr node);

private:
    class ObjectData;
    ObjectData *data_;
};

} // namespace xscript

#endif // _XSCRIPT_OBJECT_H_
