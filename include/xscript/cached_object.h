#ifndef _XSCRIPT_CACHE_OBJECT_H_
#define _XSCRIPT_CACHE_OBJECT_H_

#include <string>

#include <boost/noncopyable.hpp>

namespace xscript {

class Context;

class CachedObject : private boost::noncopyable {
public:
    CachedObject();
    virtual ~CachedObject();

    virtual std::string createTagKey(const Context *ctx) const = 0;
    virtual bool allowDistributed() const;
    
protected:
    enum Strategy {
        LOCAL,
        DISTRIBUTED,
        SMART
    };
    
    virtual bool checkProperty(const char *name, const char *value);
    Strategy strategy() const;
    
private:
    class ObjectData;
    ObjectData *data_;
};

} // namespace xscript

#endif // _XSCRIPT_CACHE_OBJECT_H_
