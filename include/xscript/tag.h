#ifndef _XSCRIPT_TAG_H_
#define _XSCRIPT_TAG_H_

#include <ctime>

namespace xscript {

class Tag {
public:
    Tag();
    Tag(bool mod, time_t last_mod, time_t exp_time);

    bool expired() const;
    static const time_t UNDEFINED_TIME;

    bool needPrefetch(time_t stored_time) const;
    
    bool valid() const;

public:
    bool modified;
    time_t last_modified, expire_time;
};

} // namespace xscript

#endif // _XSCRIPT_TAG_H_
