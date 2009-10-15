#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/config.h"
#include "xscript/context.h"
#include "xscript/doc_cache.h"
#include "internal/extension_list.h"
#include "internal/loader.h"
#include "xscript/logger_factory.h"
#include "xscript/script.h"
#include "xscript/script_factory.h"
#include "xscript/state.h"
#include "xscript/tagged_block.h"

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

        boost::shared_ptr<Request> request(new Request());
        boost::shared_ptr<Response> response(new Response());
        boost::shared_ptr<State> state(new State());
        boost::shared_ptr<Script> script = ScriptFactory::createScript("http-local-tagged.xml"); //cache_time==5
        boost::shared_ptr<Context> ctx(new Context(script, state, request, response));
        ContextStopper ctx_stopper(ctx);

        XmlDocHelper doc(script->invoke(ctx));
        CPPUNIT_ASSERT(NULL != doc.get());

        const TaggedBlock* block = dynamic_cast<const TaggedBlock*>(script->block(0));
        CPPUNIT_ASSERT(NULL != block);
        CPPUNIT_ASSERT(block->tagged());

        DocCache* tcache = DocCache::instance();

        Tag tag_load;
        XmlDocHelper doc_load;
    
        time_t now = time(NULL);
        Tag tag(true, now, now + 2);

        // check save
        CPPUNIT_ASSERT(tcache->saveDoc(ctx.get(), block, tag, doc));
        CPPUNIT_ASSERT(NULL != doc.get());

        // check load
        CPPUNIT_ASSERT(tcache->loadDoc(ctx.get(), block, tag_load, doc_load));
        CPPUNIT_ASSERT(NULL != doc_load.get());

        sleep(3);
        CPPUNIT_ASSERT(!tcache->loadDoc(ctx.get(), block, tag_load, doc_load));
    }


};

CPPUNIT_TEST_SUITE_REGISTRATION( DocCacheMemcachedTest );
