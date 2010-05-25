#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/config.h"
#include "xscript/context.h"
#include "xscript/doc_cache.h"
#include "xscript/logger_factory.h"
#include "xscript/script.h"
#include "xscript/script_factory.h"
#include "xscript/state.h"
#include "xscript/tagged_block.h"
#include "xscript/test_utils.h"

#include "internal/extension_list.h"
#include "internal/loader.h"

using namespace xscript;

class DocCacheMemcachedTest : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(DocCacheMemcachedTest);

    // This test works, but it crashes everything...
    // CPPUNIT_TEST_EXCEPTION(testNoServers, std::exception);
   
    CPPUNIT_TEST(testSaveLoad);
    

    CPPUNIT_TEST_SUITE_END();
private:

    void testNoServers() {
        try {
            std::auto_ptr<Config> config = Config::create("memcached-noservers.conf");
            config->startup();
        }
        catch(...) {
        }
    }

    void testSaveLoad() {
        std::auto_ptr<Config> config = Config::create("test.conf");
        config->startup();

        boost::shared_ptr<Context> ctx = TestUtils::createEnv("file-local-tagged.xml");
        ContextStopper ctx_stopper(ctx);
        XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
        CPPUNIT_ASSERT(NULL != doc->get());
        Block* block_tmp = const_cast<Block*>(ctx->script()->block(0));
        TaggedBlock* block = dynamic_cast<TaggedBlock*>(block_tmp);
        CPPUNIT_ASSERT(NULL != block);
        CPPUNIT_ASSERT(block->tagged());

        DocCache* tcache = DocCache::instance();

        Tag tag_load;
    
        time_t now = time(NULL);
        Tag tag(true, now, now + 2);

        // check save

        Meta meta;
        meta.set2Core("key1", "value1");
        meta.set2Core("key2", "value2");

        boost::shared_ptr<BlockCacheData> saved(new BlockCacheData(doc, meta.getCore()));
        
        CacheContext cache_ctx(block, ctx.get());
        CPPUNIT_ASSERT(tcache->saveDoc(NULL, &cache_ctx, tag, saved));
        CPPUNIT_ASSERT(NULL != doc->get());

        // check load
        
        boost::shared_ptr<BlockCacheData> loaded =
            tcache->loadDoc(NULL, &cache_ctx, tag_load);

        CPPUNIT_ASSERT(NULL != loaded.get());
        CPPUNIT_ASSERT(NULL != loaded->doc()->get());
        CPPUNIT_ASSERT(NULL != loaded->meta().get());

        Meta loaded_meta;
        loaded_meta.setCore(loaded->meta());

        CPPUNIT_ASSERT(loaded_meta.has("key1"));
        CPPUNIT_ASSERT(loaded_meta.get("key1", "") == "value1");

        CPPUNIT_ASSERT(loaded_meta.has("key2"));
        CPPUNIT_ASSERT(loaded_meta.get("key2", "") == "value2");

        CPPUNIT_ASSERT(!loaded_meta.has("key3"));

        sleep(3);
        loaded = tcache->loadDoc(NULL, &cache_ctx, tag_load);
        CPPUNIT_ASSERT(NULL == loaded.get());
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(DocCacheMemcachedTest);
