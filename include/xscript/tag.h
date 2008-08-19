#ifndef _XSCRIPT_TAG_H_
#define _XSCRIPT_TAG_H_

#include <string>
#include <ctime>

namespace xscript {

/**
 * Description of status of document for interaction with caches.
 *
 * TODO: Add detailed description and usage patterns.
 */
class Tag {
public:
    Tag();
    Tag(bool mod, time_t last_mod, time_t exp_time);

    bool expired() const;
    static const time_t UNDEFINED_TIME;

    bool needPrefetch(time_t stored_time) const;

public:
    bool modified;
    time_t last_modified, expire_time;

    // If doc was successfully loaded tagKey will hold stringified tagKey
    std::string tagKey;
};

} // namespace xscript

#endif // _XSCRIPT_TAG_H_
