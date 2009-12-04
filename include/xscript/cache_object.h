#ifndef _XSCRIPT_CACHE_OBJECT_H_
#define _XSCRIPT_CACHE_OBJECT_H_

#include <string>

#include <boost/noncopyable.hpp>

namespace xscript {

class Context;

class CacheObject : private boost::noncopyable {
public:
    CacheObject();
    virtual ~CacheObject();

    virtual std::string createTagKey(const Context *ctx) const = 0;
    virtual bool allowDistributed() const;
    
protected:
    virtual bool checkProperty(const char *name, const char *value);
    
private:
    class ObjectData;
    ObjectData *data_;
};

} // namespace xscript

#endif // _XSCRIPT_CACHE_OBJECT_H_
