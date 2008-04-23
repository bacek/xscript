#ifndef _XSCRIPT_MEMORY_STATISTIC_H_
#define _XSCRIPT_MEMORY_STATISTIC_H_

#include <utility>

namespace xscript
{

	/**
	 * Use TLS to calculate memory allocaltion statistic for libxml
	 */

	/**
	 * Init statistic
	 */
	void initAllocationStatictic();


	/**
	 * Get count of allocated memory.
	 */
	size_t getAllocatedMemory();

}

#endif
