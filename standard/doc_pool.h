#ifndef _XSCRIPT_STANDARD_DOC_POOL_H_
#define _XSCRIPT_STANDARD_DOC_POOL_H_

#include <string>
#include <map>
#include <list>

#include <boost/noncopyable.hpp>

#include "xscript/tag.h"
#include "xscript/doc_cache_strategy.h"
#include "xscript/xml_helpers.h"
#include "xscript/cache_counter.h"
#include "xscript/memory_statistic.h"

namespace xscript
{

	/**
	 * Class for storing documents in memory.
	 * Combination of hash and LRU list. Will store documents available
	 * by key lookup. On "overflow" use LRU strategy to discard old documents.
	 */
	class DocPool : private boost::noncopyable
	{
	public:
		/**
		 * Construct pool 
		 * \param capacity Maximum number of documents to store.
		 * \param name Tag name for statistic gathering.
		 */
		DocPool(size_t capacity, const std::string& name);
		virtual ~DocPool();

		/**
		 * Result of loading document.
		 */
		enum LoadResult {
			LOAD_SUCCESSFUL,
			LOAD_NOT_FOUND,
			LOAD_EXPIRED,
			LOAD_NEED_PREFETCH,
		};

		bool loadDoc(const TagKey &key, Tag &tag, XmlDocHelper &doc);
		LoadResult loadDocImpl(const std::string &keyStr, Tag &tag, XmlDocHelper &doc);

		/**
		 * Result of saving doc.
		 */
		enum SaveResult {
			SAVE_STORED,
			SAVE_UPDATED,
		};

		bool saveDoc(const TagKey &key, const Tag& tag, const XmlDocHelper &doc);
		SaveResult saveDocImpl(const std::string &keyStr, const Tag& tag, const XmlDocHelper &doc);

		void clear();

		const CacheCounter* getCounter() const;
		const AverageCounter* getMemoryCounter() const;

	private:
		class DocData;
		typedef std::map<std::string, DocData> Key2Data;
		typedef std::list<Key2Data::iterator> LRUList;

		class DocData
		{
		public:
			DocData();
			explicit DocData(LRUList::iterator list_pos);

			void assign(const Tag& tag, const xmlDocPtr ptr);

			xmlDocPtr copyDoc() const;

			void clearDoc();

		public:
			Tag					tag;
			xmlDocPtr			ptr;
			LRUList::iterator	pos;
			time_t				stored_time;
			bool				prefetch_marked;
			size_t				doc_size;
		};

		void shrink();
		void removeExpiredDocuments();

		void saveAtIterator(const Key2Data::iterator& i, const Tag& tag, const XmlDocHelper& doc);

	private:
		size_t			capacity_;
		CacheCounter	counter_;
        std::auto_ptr<AverageCounter> memoryCounter_;

		boost::mutex	mutex_;

		Key2Data		key2data_;
		LRUList			list_;
	};


}

#endif
