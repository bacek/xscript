#ifndef _XSCRIPT_TAGGABLE_H
#define _XSCRIPT_TAGGABLE_H

#include <string>

namespace xscript
{

class Context;

/**
 * Interface for cache taggable objects. E.g. TaggedBlock and Script
 */
class Taggable {
public:
    
    /**
     * Create tag key for caching.
     */
    virtual std::string createTagKey(const Context *ctx) const = 0;
    
    /**
     * Check that object actually tagged
     */
    virtual bool tagged() const = 0;

    /**
     * Set tagged state to object
     */
    virtual void tagged(bool value) = 0;
};

}

#endif
