#ifndef _XSCRIPT_DETAILS_DUMMY_SCRIPT_CACHE_H_
#define _XSCRIPT_DETAILS_DUMMY_SCRIPT_CACHE_H_

#include "xscript/script_cache.h"

namespace xscript {

class DummyScriptCache : public ScriptCache {
public:
    virtual void clear();
    virtual void erase(const std::string &name);

    virtual boost::shared_ptr<Script> fetch(const std::string &name);
    virtual void store(const std::string &name, const boost::shared_ptr<Script> &xml);

    virtual boost::mutex* getMutex(const std::string &name);
};

}

#endif
