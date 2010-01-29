#ifndef _XSCRIPT_SCRIPT_CACHE_H_
#define _XSCRIPT_SCRIPT_CACHE_H_

#include <string>

#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>

#include <xscript/component.h>
#include <xscript/stat_builder.h>

namespace xscript {

class Script;

class ScriptCache : public Component<ScriptCache> {
public:
    ScriptCache();

    virtual void clear();

    virtual boost::shared_ptr<Script> fetch(const std::string &name);
    virtual void store(const std::string &name, const boost::shared_ptr<Script> &xml);

    virtual boost::mutex* getMutex(const std::string &name);
    
protected:
    StatBuilder& statBuilder();
    
private:
    StatBuilder statBuilder_;
};

} // namespace xscript

#endif // _XSCRIPT_SCRIPT_CACHE_H_
