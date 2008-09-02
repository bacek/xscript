#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/config.h"
#include "xscript/logger_factory.h"
#include "xscript/util.h"

#include "doc_pool.h"

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

        XmlDocHelper doc1(xmlNewDoc(BAD_CAST "1.0"));

        time_t t = time(0);
        Tag tag(false, t, t+6);

        pool.saveDocImpl(key, tag, doc1);

        XmlDocHelper loaded;
        DocPool::LoadResult res = pool.loadDocImpl(key, tag, loaded);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("First load successful", DocPool::LOAD_SUCCESSFUL, res);

        realtimeSleep(5);
        res = pool.loadDocImpl(key, tag, loaded);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Awaiting prefetch successful", DocPool::LOAD_NEED_PREFETCH, res);

        realtimeSleep(2);
        res = pool.loadDocImpl(key, tag, loaded);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Expired checked", DocPool::LOAD_EXPIRED, res);

    }

    void testManyDocuments() {
        std::auto_ptr<Config> config = Config::create("test.conf");
        LoggerFactory::instance()->init(config.get());

        DocPool pool(2, "pool");

        XmlDocHelper doc1(xmlNewDoc(BAD_CAST "1.0"));

        time_t t = time(0);
        Tag tag(false, t, t+6);

        pool.saveDocImpl("1", tag, doc1);

        XmlDocHelper loaded;

        DocPool::LoadResult res = pool.loadDocImpl("1", tag, loaded);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("First load successful", DocPool::LOAD_SUCCESSFUL, res);

        pool.saveDocImpl("2", tag, doc1);
        pool.saveDocImpl("3", tag, doc1);

        // First doc should be removed from cache
        res = pool.loadDocImpl("1", tag, loaded);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Load of first document failed", DocPool::LOAD_NOT_FOUND, res);

        res = pool.loadDocImpl("2", tag, loaded);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Load of second document successful", DocPool::LOAD_SUCCESSFUL, res);
        res = pool.loadDocImpl("3", tag, loaded);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Load of third document successful", DocPool::LOAD_SUCCESSFUL, res);

        // Load second doc to move it in front of queue.
        res = pool.loadDocImpl("2", tag, loaded);

        // Save new doc. Third doc should be removed.
        pool.saveDocImpl("4", tag, doc1);

        res = pool.loadDocImpl("2", tag, loaded);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Load of second document successful", DocPool::LOAD_SUCCESSFUL, res);
        res = pool.loadDocImpl("3", tag, loaded);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Load of third document failed", DocPool::LOAD_NOT_FOUND, res);

        res = pool.loadDocImpl("4", tag, loaded);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Load of forth document successful", DocPool::LOAD_SUCCESSFUL, res);

    }

};

CPPUNIT_TEST_SUITE_REGISTRATION( DocPoolTest );
