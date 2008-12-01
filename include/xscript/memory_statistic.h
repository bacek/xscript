#ifndef _XSCRIPT_MEMORY_STATISTIC_H_
#define _XSCRIPT_MEMORY_STATISTIC_H_

#include <cstddef>

namespace xscript {

/**
 * Use TLS to calculate memory allocaltion statistic for libxml
 */

/**
 * Init statistic
 */
void initAllocationStatistic();

/**
 * Get count of allocated memory.
 */
size_t getAllocatedMemory();

class MemoryStatisticRegisterer {
public:
    MemoryStatisticRegisterer() {
        statistic_enable_ = true;
    }
    static bool statistic_enable_;
};

} // namespace xscript

#endif // _XSCRIPT_MEMORY_STATISTIC_H_
