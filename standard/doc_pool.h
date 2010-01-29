#ifndef _XSCRIPT_STANDARD_DOC_POOL_H_
#define _XSCRIPT_STANDARD_DOC_POOL_H_

#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include "internal/lrucache.h"

namespace xscript {

class CacheCounter;
class CacheData;
class Tag;

class DocPool : private boost::noncopyable {
public:
    /**
     * Construct pool
     * \param capacity Maximum number of documents to store.
     * \param name Tag name for statistic gathering.
     */
    DocPool(size_t size, const std::string &name);
    virtual ~DocPool();

    bool loadDoc(const std::string &key, Tag &tag, boost::shared_ptr<CacheData> &cache_data);
    bool saveDoc(const std::string &key, const Tag &tag, const boost::shared_ptr<CacheData> &cache_data);

    void clear();
    const CacheCounter* getCounter() const;

private:    
    LRUCache<std::string, boost::shared_ptr<CacheData> > cache_;
};


} // namespace xscript

#endif // _XSCRIPT_STANDARD_DOC_POOL_H_
