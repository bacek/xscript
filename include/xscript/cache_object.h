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
};

} // namespace xscript

#endif // _XSCRIPT_CACHE_OBJECT_H_
