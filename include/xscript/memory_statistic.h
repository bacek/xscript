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

static bool statistic_enable = false;

struct MemoryStatisticRegisterer {
    MemoryStatisticRegisterer() {
        statistic_enable = true;
    }
};

} // namespace xscript

#endif // _XSCRIPT_MEMORY_STATISTIC_H_
