#include "settings.h"

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/config.h"
#include "xscript/context.h"
#include "xscript/doc_cache.h"
#include "xscript/script.h"
#include "xscript/script_factory.h"
#include "xscript/state.h"
#include "xscript/tagged_block.h"
#include "xscript/test_utils.h"
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
//    CPPUNIT_TEST(testStoreLoad);
//    CPPUNIT_TEST(testGetLocalTagged);
//    CPPUNIT_TEST(testGetLocalTaggedPrefetch);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(DocCacheTest, "tagged-cache");
CPPUNIT_REGISTRY_ADD("tagged-cache", "xscript");

void
DocCacheTest::testMissed() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("http-local.xml");
    ContextStopper ctx_stopper(ctx);

    const TaggedBlock* block = dynamic_cast<const TaggedBlock*>(ctx->script()->block(0));
    CPPUNIT_ASSERT(NULL != block);
    CPPUNIT_ASSERT(!block->tagged());

    DocCache* tcache = DocCache::instance();

    Tag tag_load;
    XmlDocSharedHelper doc_load;

    CacheContext cache_ctx(block);
    CPPUNIT_ASSERT(!tcache->loadDoc(ctx.get(), &cache_ctx, tag_load, doc_load));
}

void
DocCacheTest::testStoreLoad() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("http-local.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc->get());

    const TaggedBlock* block = dynamic_cast<const TaggedBlock*>(ctx->script()->block(0));
    CPPUNIT_ASSERT(NULL != block);
    CPPUNIT_ASSERT(!block->tagged());

    time_t now = time(NULL);
    Tag tag(true, now, now + 2), tag_load;

    DocCache* tcache = DocCache::instance();

    // check first save
    CacheContext cache_ctx(block);
    CPPUNIT_ASSERT(tcache->saveDoc(ctx.get(), &cache_ctx, tag, doc));
    CPPUNIT_ASSERT(NULL != doc.get());

    // check save again
    CPPUNIT_ASSERT(tcache->saveDoc(ctx.get(), &cache_ctx, tag, doc));
    CPPUNIT_ASSERT(NULL != doc.get());

    // check first load
    XmlDocSharedHelper doc_load;
    CPPUNIT_ASSERT(tcache->loadDoc(ctx.get(), &cache_ctx, tag_load, doc_load));
    CPPUNIT_ASSERT(NULL != doc_load->get());

    CPPUNIT_ASSERT_EQUAL(tag.modified, tag_load.modified);
    CPPUNIT_ASSERT_EQUAL(tag.last_modified, tag_load.last_modified);
    CPPUNIT_ASSERT_EQUAL(tag.expire_time, tag_load.expire_time);

    // check load again
    doc_load.reset();
    CPPUNIT_ASSERT(tcache->loadDoc(ctx.get(), &cache_ctx, tag_load, doc_load));

    sleep(tag.expire_time - tag.last_modified);

    // check skip expired
    CPPUNIT_ASSERT(!tcache->loadDoc(ctx.get(), &cache_ctx, tag_load, doc_load));
}

void
DocCacheTest::testGetLocalTagged() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("http-local-tagged.xml");
    ContextStopper ctx_stopper(ctx);

    const TaggedBlock* block = dynamic_cast<const TaggedBlock*>(ctx->script()->block(0));
    CPPUNIT_ASSERT(NULL != block);
    CPPUNIT_ASSERT(block->tagged());

    DocCache* tcache = DocCache::instance();

    Tag tag_load;
    XmlDocSharedHelper doc_load;
    CacheContext cache_ctx(block);
    CPPUNIT_ASSERT(!tcache->loadDoc(ctx.get(), &cache_ctx, tag_load, doc_load));

    /*
    struct timeb t;
    ftime(&t);
    if (t.millitm > 300) {
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 1000000 - t.millitm * 1000;
        nanosleep(&ts, NULL);
    }
    */

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc->get());
    CPPUNIT_ASSERT(tcache->loadDoc(ctx.get(), &cache_ctx, tag_load, doc_load));
    CPPUNIT_ASSERT(NULL != doc_load->get());

    sleep(3);

    CPPUNIT_ASSERT(tcache->loadDoc(ctx.get(), &cache_ctx, tag_load, doc_load));
    CPPUNIT_ASSERT(NULL != doc_load->get());

    sleep(2);

    // check skip expired
    CPPUNIT_ASSERT(!tcache->loadDoc(ctx.get(), &cache_ctx, tag_load, doc_load));
}

void
DocCacheTest::testGetLocalTaggedPrefetch() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("http-local-tagged.xml");
    ContextStopper ctx_stopper(ctx);

    const TaggedBlock* block = dynamic_cast<const TaggedBlock*>(ctx->script()->block(0));
    CPPUNIT_ASSERT(NULL != block);
    CPPUNIT_ASSERT(block->tagged());

    DocCache* tcache = DocCache::instance();

    Tag tag_load;
    XmlDocSharedHelper doc_load;
    CacheContext cache_ctx(block);
    CPPUNIT_ASSERT(!tcache->loadDoc(ctx.get(), &cache_ctx, tag_load, doc_load));

    /*
    struct timeb t;
    ftime(&t);
    if (t.millitm > 300) {
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 1000000 - t.millitm * 1000;
        nanosleep(&ts, NULL);
    }
    */

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc->get());

    CPPUNIT_ASSERT(tcache->loadDoc(ctx.get(), &cache_ctx, tag_load, doc_load));
    CPPUNIT_ASSERT(NULL != doc_load->get());

    sleep(3);

    CPPUNIT_ASSERT(tcache->loadDoc(ctx.get(), &cache_ctx, tag_load, doc_load));
    CPPUNIT_ASSERT(NULL != doc_load->get());

    sleep(1);

    // check mark cache file for prefetch
    CPPUNIT_ASSERT(!tcache->loadDoc(ctx.get(), &cache_ctx, tag_load, doc_load));

    CPPUNIT_ASSERT(tcache->loadDoc(ctx.get(), &cache_ctx, tag_load, doc_load));
    CPPUNIT_ASSERT(tcache->loadDoc(ctx.get(), &cache_ctx, tag_load, doc_load));
    CPPUNIT_ASSERT(tcache->loadDoc(ctx.get(), &cache_ctx, tag_load, doc_load));

    sleep(1);

    // check skip expired
    CPPUNIT_ASSERT(!tcache->loadDoc(ctx.get(), &cache_ctx, tag_load, doc_load));
}

