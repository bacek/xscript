#include "settings.h"

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/state.h"
#include "xscript/script.h"
#include "xscript/config.h"
#include "xscript/context.h"
#include "xscript/doc_cache.h"
#include "xscript/tagged_block.h"
#include "xscript/request_data.h"
#include "xscript/util.h"

#include <time.h>
#include <sys/timeb.h>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

class DocCacheTest : public CppUnit::TestFixture {
public:
    void testMissed();
    void testStoreLoad();
    void testGetLocalTagged();
    void testGetLocalTaggedPrefetch();

    // TODO Create mockup strategy to check, that loaded doc from second
    // strategy was stored in first.

private:
    CPPUNIT_TEST_SUITE(DocCacheTest);
    CPPUNIT_TEST(testMissed);
    CPPUNIT_TEST(testStoreLoad);
    CPPUNIT_TEST(testGetLocalTagged);
    CPPUNIT_TEST(testGetLocalTaggedPrefetch);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(DocCacheTest, "tagged-cache");
CPPUNIT_REGISTRY_ADD("tagged-cache", "xscript");

void
DocCacheTest::testMissed() {

    using namespace xscript;

    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = Script::create("http-local.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    const TaggedBlock* block = dynamic_cast<const TaggedBlock*>(script->block(0));
    CPPUNIT_ASSERT(NULL != block);
    CPPUNIT_ASSERT(!block->tagged());

    DocCache* tcache = DocCache::instance();

    Tag tag_load;
    XmlDocHelper doc_load;

    CPPUNIT_ASSERT(!tcache->loadDoc(ctx.get(), block, tag_load, doc_load));
}

void
DocCacheTest::testStoreLoad() {

    using namespace xscript;

    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = Script::create("http-local.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    const TaggedBlock* block = dynamic_cast<const TaggedBlock*>(script->block(0));
    CPPUNIT_ASSERT(NULL != block);
    CPPUNIT_ASSERT(!block->tagged());

    time_t now = time(NULL);
    Tag tag(true, now, now + 2), tag_load;

    DocCache* tcache = DocCache::instance();

    // check first save
    CPPUNIT_ASSERT(tcache->saveDoc(ctx.get(), block, tag, doc));
    CPPUNIT_ASSERT(NULL != doc.get());

    // check save again
    CPPUNIT_ASSERT(tcache->saveDoc(ctx.get(), block, tag, doc));
    CPPUNIT_ASSERT(NULL != doc.get());

    // check first load
    XmlDocHelper doc_load;
    CPPUNIT_ASSERT(tcache->loadDoc(ctx.get(), block, tag_load, doc_load));
    CPPUNIT_ASSERT(NULL != doc_load.get());

    CPPUNIT_ASSERT_EQUAL(tag.modified, tag_load.modified);
    CPPUNIT_ASSERT_EQUAL(tag.last_modified, tag_load.last_modified);
    CPPUNIT_ASSERT_EQUAL(tag.expire_time, tag_load.expire_time);

    // check load again
    doc_load.reset(NULL);
    CPPUNIT_ASSERT(tcache->loadDoc(ctx.get(), block, tag_load, doc_load));

    realtimeSleep(tag.expire_time - tag.last_modified);

    // check skip expired
    CPPUNIT_ASSERT(!tcache->loadDoc(ctx.get(), block, tag_load, doc_load));
}

void
DocCacheTest::testGetLocalTagged() {

    using namespace xscript;

    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = Script::create("http-local-tagged.xml"); //cache_time==5
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    const TaggedBlock* block = dynamic_cast<const TaggedBlock*>(script->block(0));
    CPPUNIT_ASSERT(NULL != block);
    CPPUNIT_ASSERT(block->tagged());

    DocCache* tcache = DocCache::instance();

    Tag tag_load;
    XmlDocHelper doc_load;
    CPPUNIT_ASSERT(!tcache->loadDoc(ctx.get(), block, tag_load, doc_load));

    struct timeb t;
    ftime(&t);
    if (t.millitm > 300) {
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 1000000 - t.millitm * 1000;
        nanosleep(&ts, NULL);
    }

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());
    CPPUNIT_ASSERT(tcache->loadDoc(ctx.get(), block, tag_load, doc_load));
    CPPUNIT_ASSERT(NULL != doc_load.get());

    realtimeSleep(3);

    CPPUNIT_ASSERT(tcache->loadDoc(ctx.get(), block, tag_load, doc_load));
    CPPUNIT_ASSERT(NULL != doc_load.get());

    realtimeSleep(2);

    // check skip expired
    CPPUNIT_ASSERT(!tcache->loadDoc(ctx.get(), block, tag_load, doc_load));
}

void
DocCacheTest::testGetLocalTaggedPrefetch() {

    using namespace xscript;

    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = Script::create("http-local-tagged.xml"); //cache_time==5
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    const TaggedBlock* block = dynamic_cast<const TaggedBlock*>(script->block(0));
    CPPUNIT_ASSERT(NULL != block);
    CPPUNIT_ASSERT(block->tagged());

    DocCache* tcache = DocCache::instance();

    Tag tag_load;
    XmlDocHelper doc_load;
    CPPUNIT_ASSERT(!tcache->loadDoc(ctx.get(), block, tag_load, doc_load));

    struct timeb t;
    ftime(&t);
    if (t.millitm > 300) {
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 1000000 - t.millitm * 1000;
        nanosleep(&ts, NULL);
    }

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    CPPUNIT_ASSERT(tcache->loadDoc(ctx.get(), block, tag_load, doc_load));
    CPPUNIT_ASSERT(NULL != doc_load.get());

    realtimeSleep(3);

    CPPUNIT_ASSERT(tcache->loadDoc(ctx.get(), block, tag_load, doc_load));
    CPPUNIT_ASSERT(NULL != doc_load.get());

    realtimeSleep(1);

    // check mark cache file for prefetch
    CPPUNIT_ASSERT(!tcache->loadDoc(ctx.get(), block, tag_load, doc_load));

    CPPUNIT_ASSERT(tcache->loadDoc(ctx.get(), block, tag_load, doc_load));
    CPPUNIT_ASSERT(tcache->loadDoc(ctx.get(), block, tag_load, doc_load));
    CPPUNIT_ASSERT(tcache->loadDoc(ctx.get(), block, tag_load, doc_load));

    realtimeSleep(1);

    // check skip expired
    CPPUNIT_ASSERT(!tcache->loadDoc(ctx.get(), block, tag_load, doc_load));
}

