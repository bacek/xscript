#ifndef _XSCRIPT_DOC_CACHE_STRATEGY_H_
#define _XSCRIPT_DOC_CACHE_STRATEGY_H_

#include <memory>
#include <vector>
#include <boost/utility.hpp>

#include <xscript/component.h>
#include <xscript/xml_helpers.h>
#include <xscript/stat_builder.h>
#include <xscript/average_counter.h>

namespace xscript {

class Tag;
class Context;
class Taggable;

class DocCacheStrategy {
public:
    DocCacheStrategy();
    virtual ~DocCacheStrategy();

    virtual void init(const Config *config);

    virtual time_t minimalCacheTime() const = 0;

    virtual bool loadDoc(const std::string &tagKey, Tag &tag, XmlDocHelper &doc);
    virtual bool saveDoc(const std::string &tagKey, const Tag& tag, const XmlDocHelper &doc);

    /**
     * Create aggregate report. Caller must free result.
     */
    XmlDocHelper createReport() const {
        return statBuilder_.createReport();
    }

protected:
    virtual bool loadDocImpl(const std::string &tagKey, Tag &tag, XmlDocHelper &doc) = 0;
    virtual bool saveDocImpl(const std::string &tagKey, const Tag& tag, const XmlDocHelper &doc) = 0;

    StatBuilder statBuilder_;
    std::auto_ptr<AverageCounter> hitCounter_;
    std::auto_ptr<AverageCounter> missCounter_;
    std::auto_ptr<AverageCounter> saveCounter_;
};

} // namespace xscript

#endif // _XSCRIPT_DOC_CACHE_STRATEGY_H_
