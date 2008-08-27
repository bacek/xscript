#ifndef _XSCRIPT_INVOKE_RESULT_
#define _XSCRIPT_INVOKE_RESULT_

#include <boost/shared_ptr.hpp>
#include <xscript/xml_helpers.h>

namespace xscript
{

/**
 * Result of block invokation
 * Contains doc. 
 * Second that we fetched cached data.
 * TODO. Change completely to return closure that will fetch document
 * from cache in case of whole page cache.
 */
struct InvokeResult
{
    InvokeResult() : cached(false) {}
    /// Construct non-cached result
    explicit InvokeResult(XmlDocHelper & d) 
        : cached(false)
    {
        // We takes ownership of xmlDoc.
        doc = boost::shared_ptr<XmlDocHelper>(new XmlDocHelper(d));
        d.release();
    }

    InvokeResult(XmlDocHelper & d, bool c, const std::string &key) 
        : cached(c), tagKey(key)
    {
        // We takes ownership of xmlDoc.
        doc = boost::shared_ptr<XmlDocHelper>(new XmlDocHelper(d));
        d.release();
    }

    xmlDocPtr get() const {
        if(doc.get())
            return doc.get()->get();
        return NULL;
    }

    boost::shared_ptr<XmlDocHelper> doc;
    bool            cached;
    std::string     tagKey;
};

}

#endif