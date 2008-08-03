#ifndef _XSCRIPT_DETAILS_DUMMY_CACHE_COUNTER_H
#define _XSCRIPT_DETAILS_DUMMY_CACHE_COUNTER_H


#include "settings.h"
#include "xscript/cache_counter.h"

namespace xscript
{
    /**
     * Do nothing counter
     */
	class DummyCacheCounter : public CacheCounter
	{
	public:
		void incUsedMemory(size_t amount);
		void decUsedMemory(size_t amount);

		void incLoaded();
		void incStored();
		void incRemoved();

		virtual XmlNodeHelper createReport() const;
	};

    class DummyCacheCounterFactory : public CacheCounterFactory {
    public:
        DummyCacheCounterFactory();
        ~DummyCacheCounterFactory();

        virtual void init(const Config *config);

        std::auto_ptr<CacheCounter> createCounter(const std::string& name);
    };

}

#endif



