#ifndef _XSCRIPT_CACHE_COUNTER_
#define _XSCRIPT_CACHE_COUNTER_

#include <string>
#include <xscript/counter_base.h>

namespace xscript
{

	/**
	 * Counter for measure cache statistic.
	 * Trivial set of (loaded, stored, removed) counters.
	 */
	class CacheCounter : public CounterBase
	{
	public:
		CacheCounter(const std::string& name);
		virtual XmlNodeHelper createReport() const;

		void incUsedMemory(size_t amount) { usedMemory_ += amount; }
		void decUsedMemory(size_t amount) { usedMemory_ -= amount; }

		void incLoaded() { ++loaded_; }
		void incStored() { ++stored_; }
		void incRemoved() { ++removed_; }

	private:
        std::string name_;
		size_t usedMemory_;
		size_t stored_;
		size_t loaded_;
		size_t removed_;
	};
}

#endif
