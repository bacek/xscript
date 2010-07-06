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

    Block* block_tmp = const_cast<Block*>(ctx->script()->block(0));
    TaggedBlock* block = dynamic_cast<TaggedBlock*>(block_tmp);
    CPPUNIT_ASSERT(NULL != block);
    CPPUNIT_ASSERT(!block->tagged());

    DocCache* tcache = DocCache::instance();

    Tag tag_load;
    XmlDocSharedHelper doc_load;

    CacheContext cache_ctx(block, ctx.get());
    InvokeContext invoke_ctx;
    boost::shared_ptr<BlockCacheData> loaded = tcache->loadDoc(&invoke_ctx, &cache_ctx, tag_load);
    CPPUNIT_ASSERT(NULL == loaded.get());
}

void
DocCacheTest::testStoreLoad() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("http-local.xml");
    ContextStopper ctx_stopper(ctx);
    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());

    Block* block_tmp = const_cast<Block*>(ctx->script()->block(0));
    TaggedBlock* block = dynamic_cast<TaggedBlock*>(block_tmp);
    CPPUNIT_ASSERT(NULL != block);
    CPPUNIT_ASSERT(!block->tagged());

    time_t now = time(NULL);
    Tag tag(true, now, now + 2), tag_load;

    DocCache* tcache = DocCache::instance();

    boost::shared_ptr<BlockCacheData> saved(new BlockCacheData(
        doc, boost::shared_ptr<MetaCore>()));
    
    // check first save
    CacheContext cache_ctx(block, ctx.get());
    InvokeContext invoke_ctx;
    CPPUNIT_ASSERT(tcache->saveDoc(&invoke_ctx, &cache_ctx, tag, saved));
    CPPUNIT_ASSERT(NULL != doc.get());

    // check save again
    CPPUNIT_ASSERT(tcache->saveDoc(&invoke_ctx, &cache_ctx, tag, saved));
    CPPUNIT_ASSERT(NULL != doc.get());

    // check first load
    boost::shared_ptr<BlockCacheData> loaded = tcache->loadDoc(&invoke_ctx, &cache_ctx, tag_load);
    CPPUNIT_ASSERT(NULL != loaded.get());
    CPPUNIT_ASSERT(NULL != loaded->doc().get());

    CPPUNIT_ASSERT_EQUAL(tag.modified, tag_load.modified);
    CPPUNIT_ASSERT_EQUAL(tag.last_modified, tag_load.last_modified);
    CPPUNIT_ASSERT_EQUAL(tag.expire_time, tag_load.expire_time);

    // check load again
    loaded.reset();
    loaded = tcache->loadDoc(&invoke_ctx, &cache_ctx, tag_load);
    CPPUNIT_ASSERT(NULL != loaded.get());
    CPPUNIT_ASSERT(NULL != loaded->doc().get());

    sleep(tag.expire_time - tag.last_modified);

    // check skip expired
    loaded.reset();
    loaded = tcache->loadDoc(&invoke_ctx, &cache_ctx, tag_load);
    CPPUNIT_ASSERT(NULL == loaded.get());
}

void
DocCacheTest::testGetLocalTagged() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("file-local-tagged.xml");
    ContextStopper ctx_stopper(ctx);

    Block* block_tmp = const_cast<Block*>(ctx->script()->block(0));
    TaggedBlock* block = dynamic_cast<TaggedBlock*>(block_tmp);
    CPPUNIT_ASSERT(NULL != block);
    CPPUNIT_ASSERT(block->tagged());

    DocCache* tcache = DocCache::instance();

    Tag tag_load;
    CacheContext cache_ctx(block, ctx.get());
    InvokeContext invoke_ctx;
    boost::shared_ptr<BlockCacheData> loaded = tcache->loadDoc(&invoke_ctx, &cache_ctx, tag_load);
    CPPUNIT_ASSERT(NULL == loaded.get());

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
    CPPUNIT_ASSERT(NULL != doc.get());
    
    loaded.reset();
    loaded = tcache->loadDoc(&invoke_ctx, &cache_ctx, tag_load);
    CPPUNIT_ASSERT(NULL != loaded.get());
    CPPUNIT_ASSERT(NULL != loaded->doc().get());

    sleep(3);

    loaded.reset();
    loaded = tcache->loadDoc(&invoke_ctx, &cache_ctx, tag_load);
    CPPUNIT_ASSERT(NULL != loaded.get());
    CPPUNIT_ASSERT(NULL != loaded->doc().get());

    sleep(2);

    // check skip expired
    loaded.reset();
    loaded = tcache->loadDoc(&invoke_ctx, &cache_ctx, tag_load);
    CPPUNIT_ASSERT(NULL == loaded.get());
}

void
DocCacheTest::testGetLocalTaggedPrefetch() {
    using namespace xscript;
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("file-local-tagged.xml");
    ContextStopper ctx_stopper(ctx);

    Block* block_tmp = const_cast<Block*>(ctx->script()->block(0));
    TaggedBlock* block = dynamic_cast<TaggedBlock*>(block_tmp);
    CPPUNIT_ASSERT(NULL != block);
    CPPUNIT_ASSERT(block->tagged());

    DocCache* tcache = DocCache::instance();

    Tag tag_load;
    CacheContext cache_ctx(block, ctx.get());
    InvokeContext invoke_ctx;
    boost::shared_ptr<BlockCacheData> loaded = tcache->loadDoc(&invoke_ctx, &cache_ctx, tag_load);
    CPPUNIT_ASSERT(NULL == loaded.get());

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
    CPPUNIT_ASSERT(NULL != doc.get());

    loaded.reset();
    loaded = tcache->loadDoc(&invoke_ctx, &cache_ctx, tag_load);
    CPPUNIT_ASSERT(NULL != loaded.get());
    CPPUNIT_ASSERT(NULL != loaded->doc().get());

    sleep(3);

    loaded.reset();
    loaded = tcache->loadDoc(&invoke_ctx, &cache_ctx, tag_load);
    CPPUNIT_ASSERT(NULL != loaded.get());
    CPPUNIT_ASSERT(NULL != loaded->doc().get());

    sleep(1);

    // check mark cache file for prefetch
    loaded.reset();
    loaded = tcache->loadDoc(&invoke_ctx, &cache_ctx, tag_load);
    CPPUNIT_ASSERT(NULL != loaded.get());
    CPPUNIT_ASSERT(NULL != loaded->doc().get());

    loaded.reset();
    loaded = tcache->loadDoc(&invoke_ctx, &cache_ctx, tag_load);
    CPPUNIT_ASSERT(NULL == loaded.get());
    
    loaded.reset();
    loaded = tcache->loadDoc(&invoke_ctx, &cache_ctx, tag_load);
    CPPUNIT_ASSERT(NULL == loaded.get());
    
    loaded.reset();
    loaded = tcache->loadDoc(&invoke_ctx, &cache_ctx, tag_load);
    CPPUNIT_ASSERT(NULL == loaded.get());
    
    sleep(1);

    // check skip expired
    loaded.reset();
    loaded = tcache->loadDoc(&invoke_ctx, &cache_ctx, tag_load);
    CPPUNIT_ASSERT(NULL != loaded.get());
    CPPUNIT_ASSERT(NULL != loaded->doc().get());
}

