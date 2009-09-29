#include "settings.h"

#include <boost/bind.hpp>
#include <boost/thread.hpp>


#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class ParamsMap {
public:
    typedef std::map<std::string, boost::any> MapType;
    typedef MapType::const_iterator iterator;
    
    bool insert(const std::string &key, const boost::any &value);
    bool find(const std::string &key, boost::any &value) const;
private:
    mutable boost::mutex mutex_;
    MapType params_;
};



bool
ParamsMap::insert(const std::string &name, const boost::any &value) {
    boost::mutex::scoped_lock sl(mutex_);
    iterator it = params_.find(name);
    if (params_.end() != it) {
        return false;
    }
    params_.insert(std::make_pair(name, value));
    return true;
}

bool
ParamsMap::find(const std::string &name, boost::any &value) const {
    boost::mutex::scoped_lock sl(mutex_);
    iterator it = params_.find(name);
    if (params_.end() == it) {
        return false;
    }
    value = it->second;
    return true;
}


} // namespace xscript
