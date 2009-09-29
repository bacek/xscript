#ifndef _XSCRIPT_OBJECT_STORAGE_H_
#define _XSCRIPT_OBJECT_STORAGE_H_

#include <boost/any.hpp>

namespace xscript {

class ObjectStorage {
public:
    typedef std::map<std::string, boost::any> MapType;
    typedef MapType::const_iterator iterator;
    
    template<typename T> T get(const std::string &name) const;
    template<typename T> void insert(const std::string &name, const T &t);

    // Get or create param
    typedef boost::shared_ptr<boost::mutex> MutexPtr;
    
    template<typename T> T get(const std::string &name,
                               const boost::function<T ()> &creator,
                               boost::mutex &mutex);
    
    bool insert(const std::string &key, const boost::any &value);
    bool find(const std::string &key, boost::any &value) const;
private:
    mutable boost::mutex mutex_;
    MapType storage_;
};


    // Get or create param
    typedef boost::shared_ptr<boost::mutex> MutexPtr;
    
    template<typename T> T param(
        const std::string &name, const boost::function<T ()> &creator, boost::mutex &mutex);


template<typename T> inline T
Context::param(const std::string &name) const {
    boost::any value;
    if (findParam(name, value)) {
        return boost::any_cast<T>(value);
    }
    else {
        throw std::invalid_argument(std::string("nonexistent param: ").append(name));
    }
}

template<typename T> inline void
Context::param(const std::string &name, const T &t) {
    if (!insertParam(name, boost::any(t))) {
        throw std::invalid_argument(std::string("duplicate param: ").append(name));
    }
}

template<typename T> inline T
Context::param(const std::string &name, const boost::function<T ()> &creator, boost::mutex &mutex) {
    boost::any value;
    boost::mutex::scoped_lock lock(mutex);
    if (findParam(name, value)) {
        return boost::any_cast<T>(value);
    }
    else {
        T t = creator();
        insertParam(name, boost::any(t));
        return t;
    }
}

} // namespace xscript

#endif // _XSCRIPT_OBJECT_STORAGE_H_
