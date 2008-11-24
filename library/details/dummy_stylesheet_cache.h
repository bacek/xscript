#ifndef _XSCRIPT_DETAILS_DUMMY_STYLESHEET_CACHE_H_
#define _XSCRIPT_DETAILS_DUMMY_STYLESHEET_CACHE_H_


#include "xscript/stylesheet_cache.h"

namespace xscript {

class DummyStylesheetCache : public StylesheetCache {
public:

    virtual void clear();
    virtual void erase(const std::string &name);

    virtual boost::shared_ptr<Stylesheet> fetch(const std::string &name);
    virtual void store(const std::string &name, const boost::shared_ptr<Stylesheet> &stylesheet);

    virtual boost::mutex* getMutex(const std::string &name);
};

}

#endif
