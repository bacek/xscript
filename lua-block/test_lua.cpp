#include "settings.h"

#include <stdexcept>
#include <boost/lexical_cast.hpp>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/exception.h"
#include "xscript/http_utils.h"
#include "xscript/xml_util.h"
#include "xscript/test_utils.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

using namespace xscript;

class LuaTest : public CppUnit::TestFixture {
public:   
    void testPrint();

    void testState();
    void testStateHas();
    void testStateIs();
    void testRequest();
    void testResponse();
    void testResponseRedirect();

    void testEncode();
    void testCookie();

    void testBadCode();
    void testBadType();
    void testBadArgCount();

    void testMultiBlock();
    void testMD5();
    void testDomain();
    void testXmlEncode();
    void testGetVHostArg();
    void testStrSplit();
    
    void testLogger();
private:
    CPPUNIT_TEST_SUITE(LuaTest);

    CPPUNIT_TEST(testPrint);

    CPPUNIT_TEST(testState);
    CPPUNIT_TEST(testStateHas);
    CPPUNIT_TEST(testStateIs);
    CPPUNIT_TEST(testRequest);
    CPPUNIT_TEST(testResponse);
    CPPUNIT_TEST(testResponseRedirect);

    CPPUNIT_TEST(testEncode);
    CPPUNIT_TEST(testCookie);

    CPPUNIT_TEST(testBadType);
    CPPUNIT_TEST(testBadArgCount);
    CPPUNIT_TEST_EXCEPTION(testBadCode, ParseError);

    CPPUNIT_TEST(testMultiBlock);
    CPPUNIT_TEST(testMD5);
    CPPUNIT_TEST(testDomain);
    CPPUNIT_TEST(testXmlEncode);
    CPPUNIT_TEST(testGetVHostArg);
    CPPUNIT_TEST(testStrSplit);

    CPPUNIT_TEST(testLogger);

    CPPUNIT_TEST_SUITE_END();
};

#ifdef HAVE_LUA_BLOCK

CPPUNIT_TEST_SUITE_REGISTRATION( LuaTest );

#endif

void
LuaTest::testPrint() {
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("lua-print.xml");
    ContextStopper ctx_stopper(ctx);

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());
    
    CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "/page/lua"));
    CPPUNIT_ASSERT_EQUAL(
        std::string("Hello\n&nbsp;\nWorld!"),
        XmlUtils::xpathValue(doc.get(), "/page/lua", "Bye")
    );
}


void
LuaTest::testState() {
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("lua-state.xml");
    ContextStopper ctx_stopper(ctx);

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());

    for (int i = 1; i <= 10; ++i) {

        std::string num = boost::lexical_cast<std::string>(i);
        CPPUNIT_ASSERT_EQUAL(ctx->state()->asString("long " + num), num);

        CPPUNIT_ASSERT_EQUAL(ctx->state()->asString("string " + num),
                             boost::lexical_cast<std::string>(i * 2));
        CPPUNIT_ASSERT_EQUAL(ctx->state()->asString("long long " + num),
                             boost::lexical_cast<std::string>(i * 3));
    }

    try {
        std::string str = ctx->state()->asString("unknown_param");
        throw std::logic_error("State must throw exception for not existed element");
    }
    catch (std::invalid_argument &) {
        //ok
    }
}

void
LuaTest::testStateHas() {
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("lua-state-has.xml");
    ContextStopper ctx_stopper(ctx);

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());

    // Unknown param returns empty string
    CPPUNIT_ASSERT_EQUAL(std::string(""), ctx->state()->asString("unknown_param"));

    CPPUNIT_ASSERT_EQUAL(std::string("has1 passed"), ctx->state()->asString("has1"));
    CPPUNIT_ASSERT_EQUAL(std::string("has2 passed"), ctx->state()->asString("has2"));

    CPPUNIT_ASSERT_EQUAL(std::string("0"), ctx->state()->asString("art"));
    CPPUNIT_ASSERT_EQUAL(std::string("0"), ctx->state()->asString("xxx_art"));
}

void
LuaTest::testStateIs() {
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("lua-state-is.xml");
    ContextStopper ctx_stopper(ctx);

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());
    
    CPPUNIT_ASSERT(ctx->state()->asBool("guard0_passed"));
    CPPUNIT_ASSERT(ctx->state()->asBool("guard1_passed"));
    CPPUNIT_ASSERT(ctx->state()->asBool("guard2_passed"));
}

void
LuaTest::testRequest() {
    
    char *env[] = {
        (char*)"QUERY_STRING=query=QUERY",
        (char*)"HTTP_HOST=fireball.yandex.ru",
        (char*)"HTTP_COOKIE=SessionId=2.12.85.0.6;",
        (char*)"HTTPS=on",
        (char*)"HTTP_CONTENT_LENGTH=42",
        (char*)"DOCUMENT_ROOT=test_doc_root",
        (char*)NULL
    };
    
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("lua-request.xml", env);
    ContextStopper ctx_stopper(ctx);

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);

    CPPUNIT_ASSERT_EQUAL(std::string("QUERY"), ctx->state()->asString("test args"));
    CPPUNIT_ASSERT_EQUAL(std::string("fireball.yandex.ru"), ctx->state()->asString("test headers"));
    CPPUNIT_ASSERT_EQUAL(std::string("2.12.85.0.6"), ctx->state()->asString("test cookies"));
    CPPUNIT_ASSERT_EQUAL(true, ctx->state()->asBool("test isSecure"));
    CPPUNIT_ASSERT_EQUAL(boost::int64_t(42), ctx->state()->asLongLong("test content_length"));
    CPPUNIT_ASSERT_EQUAL(std::string("test_doc_root"), ctx->state()->asString("test document_root"));
}

void
LuaTest::testResponse() {
    
    char *env[] = {
        (char*)"QUERY_STRING=query=QUERY",
        (char*)"HTTP_HOST=fireball.yandex.ru",
        (char*)"HTTP_COOKIE=SessionId=2.12.85.0.6;",
        (char*)NULL
    };
    
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("lua-response.xml", env);
    ContextStopper ctx_stopper(ctx);

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);

    CPPUNIT_ASSERT_EQUAL((unsigned short)404, ctx->response()->status());
    
    const HeaderMap& headers = ctx->response()->outHeaders();
    HeaderMap::const_iterator it = headers.find("X-Header");
    std::string header = it == headers.end() ? StringUtils::EMPTY_STRING : it->second;
    CPPUNIT_ASSERT_EQUAL(std::string("Foo Bar"), header);
    
    it = headers.find("Content-type");
    header = it == headers.end() ? StringUtils::EMPTY_STRING : it->second;
    CPPUNIT_ASSERT_EQUAL(std::string("application/binary"), header);
}

void
LuaTest::testResponseRedirect() {
    
    char *env[] = {
        (char*)"QUERY_STRING=query=QUERY",
        (char*)"HTTP_HOST=fireball.yandex.ru",
        (char*)"HTTP_COOKIE=SessionId=2.12.85.0.6;",
        NULL
    };
    
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("lua-response-redirect.xml", env);
    ContextStopper ctx_stopper(ctx);

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);

    CPPUNIT_ASSERT_EQUAL((unsigned short)302, ctx->response()->status());
    
    const HeaderMap& headers = ctx->response()->outHeaders();
    HeaderMap::const_iterator it = headers.find("Location");
    std::string header = it == headers.end() ? StringUtils::EMPTY_STRING : it->second;    
    
    CPPUNIT_ASSERT_EQUAL(std::string("http://example.com/"), header);

}

void
LuaTest::testEncode() {
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("lua-encode.xml");
    ContextStopper ctx_stopper(ctx);

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());
    CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "/page/lua"));
    CPPUNIT_ASSERT_EQUAL(
        std::string("%CF%F0%E5%E2%E5%E4\nПревед\n%D0%9F%D1%80%D0%B5%D0%B2%D0%B5%D0%B4\nПревед"),
        XmlUtils::xpathValue(doc.get(), "/page/lua", "Bye")
    );
}

void
LuaTest::testCookie() {
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("lua-cookie.xml");
    ContextStopper ctx_stopper(ctx);

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());
    
    const CookieSet& cookies = ctx->response()->outCookies();
    Cookie cookie("foo", "");
    CookieSet::const_iterator cookie_it = cookies.find(cookie);
    
    CPPUNIT_ASSERT(cookies.end() != cookie_it);
    CPPUNIT_ASSERT_EQUAL(std::string("/some/path"), cookie_it->path());
    CPPUNIT_ASSERT_EQUAL(std::string(".example.com"), cookie_it->domain());
    CPPUNIT_ASSERT_EQUAL(time_t(123456789), cookie_it->expires());

    std::string out = XmlUtils::xpathValue(doc.get(), "/page/lua", "Bye");
    CPPUNIT_ASSERT(out.find(".example.com") != std::string::npos);
    CPPUNIT_ASSERT(out.find( "/some/path") != std::string::npos);
    CPPUNIT_ASSERT(out.find( "123456789") != std::string::npos);

    Cookie cookie2("baz", "");
    cookie_it = cookies.find(cookie2);
    CPPUNIT_ASSERT(cookies.end() != cookie_it);
    CPPUNIT_ASSERT_EQUAL(time_t(HttpDateUtils::MAX_LIVE_TIME), cookie_it->expires());
}


void
LuaTest::testBadType() {
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("lua-badtype.xml");
    ContextStopper ctx_stopper(ctx);

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());
    CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "/page/xscript_invoke_failed"));
}

void
LuaTest::testBadArgCount() {
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("lua-badargcount.xml");
    ContextStopper ctx_stopper(ctx);

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());
    CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "/page/xscript_invoke_failed"));
}

void
LuaTest::testBadCode() {
    TestUtils::createEnv("lua-badcode.xml");
}


void
LuaTest::testMultiBlock() {
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("lua-multi.xml");
    ContextStopper ctx_stopper(ctx);

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());
    CPPUNIT_ASSERT_EQUAL(ctx->state()->asString("bar"), std::string("baz"));
}

void
LuaTest::testMD5() {
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("lua-md5.xml");
    ContextStopper ctx_stopper(ctx);

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());
    
    CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "/page/lua"));
    CPPUNIT_ASSERT_EQUAL(
        std::string("5946210c9e93ae37891dfe96c3e39614"),
        XmlUtils::xpathValue(doc.get(), "/page/lua", "foo")
    );
}

void
LuaTest::testDomain() {

    using namespace xscript;

    boost::shared_ptr<Context> ctx = TestUtils::createEnv("lua-domain.xml");
    ContextStopper ctx_stopper(ctx);

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());
    
    CPPUNIT_ASSERT_EQUAL(std::string("hghltd.yandex.net"), ctx->state()->asString("no_level"));
    CPPUNIT_ASSERT_EQUAL(std::string("net"), ctx->state()->asString("tld"));
    CPPUNIT_ASSERT_EQUAL(std::string("www.yandex.ru"), ctx->state()->asString("no_level_no_scheme"));
    CPPUNIT_ASSERT_EQUAL(std::string("yandex.ru"), ctx->state()->asString("yandex.ru"));
    CPPUNIT_ASSERT_EQUAL(std::string("yandex.ru"), ctx->state()->asString("no_scheme"));
    CPPUNIT_ASSERT_EQUAL(std::string("localhost"), ctx->state()->asString("localhost"));
    CPPUNIT_ASSERT(!ctx->state()->has("invalid"));
    CPPUNIT_ASSERT(!ctx->state()->has("localfile"));
    CPPUNIT_ASSERT(!ctx->state()->has("empty"));
}

void
LuaTest::testXmlEncode() {
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("lua-xmlescape.xml");
    ContextStopper ctx_stopper(ctx);

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());
    
    CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "/page/lua"));
    CPPUNIT_ASSERT_EQUAL(
        std::string("&lt;some &amp; value&gt;"),
        XmlUtils::xpathValue(doc.get(), "/page/lua", "foo")
    );
}

// Incomplete test for logger... Just check manually default.log
void
LuaTest::testLogger() {
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("lua-logger.xml");
    ContextStopper ctx_stopper(ctx);

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());
}

void
LuaTest::testGetVHostArg() {
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("lua-get-vhost-arg.xml");
    ContextStopper ctx_stopper(ctx);

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());

    // Unknown param returns empty string
    CPPUNIT_ASSERT_EQUAL(std::string("1111"), ctx->state()->asString("maxdepth"));
}

void
LuaTest::testStrSplit() {
    boost::shared_ptr<Context> ctx = TestUtils::createEnv("lua-strsplit.xml");
    ContextStopper ctx_stopper(ctx);

    XmlDocSharedHelper doc = ctx->script()->invoke(ctx);
    CPPUNIT_ASSERT(NULL != doc.get());

    CPPUNIT_ASSERT_EQUAL(std::string("value1"), ctx->state()->asString("var1"));
    CPPUNIT_ASSERT_EQUAL(std::string("value2"), ctx->state()->asString("var2"));
    CPPUNIT_ASSERT_EQUAL(std::string("value3"), ctx->state()->asString("var3"));
    
    CPPUNIT_ASSERT_EQUAL(ctx->result(1)->error(), true);
}
