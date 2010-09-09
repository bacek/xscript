#ifndef _XSCRIPT_CACHE_OBJECT_H_
#define _XSCRIPT_CACHE_OBJECT_H_

#include <string>
#include <vector>

#include <boost/noncopyable.hpp>

#include "xscript/xml.h"

namespace xscript {

class ArgList;
class Block;
class CacheStrategy;
class Context;
class InvokeContext;
class Param;

class CachedObject : private boost::noncopyable {
public:
    CachedObject();
    virtual ~CachedObject();

    time_t cacheTime() const;
    void cacheTime(time_t cache_time);
    
    virtual std::string createTagKey(const Context *ctx, const InvokeContext *invoke_ctx) const = 0;
    virtual bool allowDistributed() const;
   
    enum Strategy {
        UNKNOWN = 0,
        LOCAL = 1,
        DISTRIBUTED = 2
    };

    //TODO: do it virtual
    bool checkStrategy(Strategy strategy) const;
    
    CacheStrategy* cacheStrategy() const;
    
    static void addDefaultStrategy(Strategy strategy);
    static void clearDefaultStrategy(Strategy strategy);
    
    static std::string fileModifiedKey(const std::string &filename);
    static std::string blocksModifiedKey(const std::vector<Block*> &blocks);
    static std::string modifiedKey(const Xml::TimeMapType &modified_info);
    static std::string paramsKey(const ArgList *args);
    static std::string paramsIdKey(const std::vector<Param*> &params, const Context *ctx);
    
    static const time_t CACHE_TIME_UNDEFINED;
    
protected:
    virtual bool checkProperty(const char *name, const char *value);
    bool cacheTimeUndefined() const;
    
private:
    class ObjectData;
    std::auto_ptr<ObjectData> data_;
};

} // namespace xscript

#endif // _XSCRIPT_CACHE_OBJECT_H_
