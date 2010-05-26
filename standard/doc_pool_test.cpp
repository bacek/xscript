#include "settings.h"

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <boost/shared_ptr.hpp>

#include "xscript/config.h"
#include "xscript/doc_cache.h"
#include "xscript/logger_factory.h"
#include "xscript/meta.h"
#include "xscript/util.h"

#include "doc_pool.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

using namespace xscript;

class DocPoolTest : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(DocPoolTest);
    CPPUNIT_TEST(testSingleDocument);
    CPPUNIT_TEST(testManyDocuments);
    CPPUNIT_TEST_SUITE_END();

public:
    void testSingleDocument() {
        std::auto_ptr<Config> config = Config::create("test.conf");
        LoggerFactory::instance()->init(config.get());

        DocPool pool(2, "pool");

        std::string key("1");

        XmlDocSharedHelper doc1(new XmlDocHelper(xmlNewDoc((const xmlChar*) "1.0")));
        boost::shared_ptr<CacheData> saved(new BlockCacheData(
            doc1, boost::shared_ptr<Meta::Core>()));
        
        time_t t = time(0);
        Tag tag(false, t, t+6);

        DocPool::CleanupFunc cleanFunc;
        pool.saveDoc(key, tag, saved, cleanFunc);

        boost::shared_ptr<CacheData> loaded(new BlockCacheData());
        
        bool res = pool.loadDoc(key, tag, loaded, cleanFunc);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("First load successful", true, res);

        sleep(5);
        res = pool.loadDoc(key, tag, loaded, cleanFunc);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Awaiting prefetch successful", false, res);

        sleep(2);
        res = pool.loadDoc(key, tag, loaded, cleanFunc);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Expired checked", false, res);

    }

    void testManyDocuments() {
        std::auto_ptr<Config> config = Config::create("test.conf");
        LoggerFactory::instance()->init(config.get());

        log()->debug("testManyDocuments STARTED");
        
        DocPool pool(2, "pool");

        XmlDocSharedHelper doc1(new XmlDocHelper(xmlNewDoc((const xmlChar*) "1.0")));

        time_t t = time(0);
        Tag tag(false, t, t+6);

        boost::shared_ptr<CacheData> saved(new BlockCacheData(
            doc1, boost::shared_ptr<Meta::Core>()));
        
        DocPool::CleanupFunc cleanFunc;
        pool.saveDoc("1", tag, saved, cleanFunc);

        boost::shared_ptr<CacheData> loaded(new BlockCacheData());

        bool res = pool.loadDoc("1", tag, loaded, cleanFunc);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("First load successful", true, res);

        pool.saveDoc("2", tag, saved, cleanFunc);
        pool.saveDoc("3", tag, saved, cleanFunc);

        // First doc should be removed from cache
        res = pool.loadDoc("1", tag, loaded, cleanFunc);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Load of first document failed", false, res);

        res = pool.loadDoc("2", tag, loaded, cleanFunc);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Load of second document successful", true, res);
        res = pool.loadDoc("3", tag, loaded, cleanFunc);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Load of third document successful", true, res);

        // Load second doc to move it in front of queue.
        res = pool.loadDoc("2", tag, loaded, cleanFunc);

        // Save new doc. Third doc should be removed.
        pool.saveDoc("4", tag, saved, cleanFunc);

        res = pool.loadDoc("2", tag, loaded, cleanFunc);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Load of second document successful", true, res);
        res = pool.loadDoc("3", tag, loaded, cleanFunc);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Load of third document failed", false, res);

        res = pool.loadDoc("4", tag, loaded, cleanFunc);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Load of forth document successful", true, res);

        log()->debug("testManyDocuments FINISHED");
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION( DocPoolTest );
