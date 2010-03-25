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
        const TaggedBlock* block = dynamic_cast<const TaggedBlock*>(ctx->script()->block(0));
        CPPUNIT_ASSERT(NULL != block);
        CPPUNIT_ASSERT(block->tagged());

        DocCache* tcache = DocCache::instance();

        Tag tag_load;
    
        time_t now = time(NULL);
        Tag tag(true, now, now + 2);

        // check save
        
        boost::shared_ptr<BlockCacheData> saved(new BlockCacheData(doc));
        
        CacheContext cache_ctx(block);
        CPPUNIT_ASSERT(tcache->saveDoc(ctx.get(), NULL, &cache_ctx, tag, saved));
        CPPUNIT_ASSERT(NULL != doc->get());

        // check load
        
        boost::shared_ptr<BlockCacheData> loaded = tcache->loadDoc(ctx.get(), NULL, &cache_ctx, tag_load);
        CPPUNIT_ASSERT(NULL != loaded.get());
        CPPUNIT_ASSERT(NULL != loaded->doc()->get());

        sleep(3);
        loaded = tcache->loadDoc(ctx.get(), NULL, &cache_ctx, tag_load);
        CPPUNIT_ASSERT(NULL == loaded.get());
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(DocCacheMemcachedTest);
