#include "settings.h"

#include <stdexcept>
#include <boost/lexical_cast.hpp>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/context.h"
#include "xscript/request.h"
#include "xscript/request_data.h"
#include "xscript/script.h"
#include "xscript/script_factory.h"
#include "xscript/state.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

using namespace xscript;

class LuaTest : public CppUnit::TestFixture {
public:

    void attachRequest(Request *request,
                       const std::vector<std::string> &env);
    
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
    CPPUNIT_TEST_EXCEPTION(testBadCode, std::runtime_error);

    CPPUNIT_TEST(testMultiBlock);
    CPPUNIT_TEST(testMD5);
    CPPUNIT_TEST(testDomain);
    CPPUNIT_TEST(testXmlEncode);

    CPPUNIT_TEST(testLogger);

    CPPUNIT_TEST_SUITE_END();
};

#ifdef HAVE_LUA_BLOCK

CPPUNIT_TEST_SUITE_REGISTRATION( LuaTest );

#endif

void
LuaTest::attachRequest(Request *request,
                       const std::vector<std::string> &env) {   
    char* env_vars[env.size() + 1];
    env_vars[env.size()] = NULL;
    for(unsigned int i = 0; i < env.size(); ++i) {
        env_vars[i] = const_cast<char*>(env[i].c_str());
    }
    std::stringstream stream(StringUtils::EMPTY_STRING);  
    request->attach(&stream, env_vars);
}

void
LuaTest::testPrint() {
    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = ScriptFactory::createScript("lua-print.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));

    CPPUNIT_ASSERT(NULL != doc.get());
    CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "/page/lua"));
    CPPUNIT_ASSERT_EQUAL(
        std::string("Hello\n&nbsp;\nWorld!"),
        XmlUtils::xpathValue(doc.get(), "/page/lua", "Bye")
    );
}


void
LuaTest::testState() {
    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = ScriptFactory::createScript("lua-state.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    State* state = data->state();
    for (int i = 1; i <= 10; ++i) {

        std::string num = boost::lexical_cast<std::string>(i);
        CPPUNIT_ASSERT_EQUAL(state->asString("long " + num), num);

        CPPUNIT_ASSERT_EQUAL(state->asString("string " + num),
                             boost::lexical_cast<std::string>(i * 2));
        CPPUNIT_ASSERT_EQUAL(state->asString("long long " + num),
                             boost::lexical_cast<std::string>(i * 3));
    }

    try {
        std::string str = state->asString("unknown_param");
        throw std::logic_error("State must throw exception for not existed element");
    }
    catch (std::invalid_argument &) {
        //ok
    }
}

void
LuaTest::testStateHas() {
    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = ScriptFactory::createScript("lua-state-has.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    State* state = data->state();

    // Unknown param returns empty string
    CPPUNIT_ASSERT_EQUAL(std::string(""), state->asString("unknown_param"));

    CPPUNIT_ASSERT_EQUAL(std::string("has1 passed"), state->asString("has1"));
    CPPUNIT_ASSERT_EQUAL(std::string("has2 passed"), state->asString("has2"));

    CPPUNIT_ASSERT_EQUAL(std::string("0"), state->asString("art"));
    CPPUNIT_ASSERT_EQUAL(std::string("0"), state->asString("xxx_art"));
}

void
LuaTest::testStateIs() {
    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = ScriptFactory::createScript("lua-state-is.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    State* state = data->state();

    CPPUNIT_ASSERT(state->asBool("guard0_passed"));
    CPPUNIT_ASSERT(state->asBool("guard1_passed"));
    CPPUNIT_ASSERT(state->asBool("guard2_passed"));
}

void
LuaTest::testRequest() {

    boost::shared_ptr<Request> request(new Request());
    boost::shared_ptr<Response> response(new Response());   
    boost::shared_ptr<State> state(new State());

    std::vector<std::string> env;
    env.push_back("QUERY_STRING=query=QUERY");
    env.push_back("HTTP_HOST=fireball.yandex.ru");
    env.push_back("HTTP_COOKIE=SessionId=2.12.85.0.6;");
    env.push_back("HTTPS=on");
    env.push_back("HTTP_CONTENT_LENGTH=42");
    
    attachRequest(request.get(), env);
    
    boost::shared_ptr<RequestData> data(new RequestData(request, response, state));
    boost::shared_ptr<Script> script = ScriptFactory::createScript("lua-request.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));

    CPPUNIT_ASSERT_EQUAL(std::string("QUERY"), state->asString("test args"));
    CPPUNIT_ASSERT_EQUAL(std::string("fireball.yandex.ru"), state->asString("test headers"));
    CPPUNIT_ASSERT_EQUAL(std::string("2.12.85.0.6"), state->asString("test cookies"));
    CPPUNIT_ASSERT_EQUAL(true, state->asBool("test isSecure"));
    CPPUNIT_ASSERT_EQUAL(boost::int64_t(42), state->asLongLong("test content_length"));
}

void
LuaTest::testResponse() {
   
    boost::shared_ptr<Request> request(new Request());
    boost::shared_ptr<Response> response(new Response());   
    boost::shared_ptr<State> state(new State());

    std::vector<std::string> env;
    env.push_back("QUERY_STRING=query=QUERY");
    env.push_back("HTTP_HOST=fireball.yandex.ru");
    env.push_back("HTTP_COOKIE=SessionId=2.12.85.0.6;");
    
    attachRequest(request.get(), env);

    boost::shared_ptr<RequestData> data(new RequestData(request, response, state));
    boost::shared_ptr<Script> script = ScriptFactory::createScript("lua-response.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));

    CPPUNIT_ASSERT_EQUAL((unsigned short)404, response->status());
    
    const HeaderMap& headers = response->outHeaders();
    HeaderMap::const_iterator it = headers.find("X-Header");
    std::string header = it == headers.end() ? StringUtils::EMPTY_STRING : it->second;
    CPPUNIT_ASSERT_EQUAL(std::string("Foo Bar"), header);
    
    it = headers.find("Content-type");
    header = it == headers.end() ? StringUtils::EMPTY_STRING : it->second;
    CPPUNIT_ASSERT_EQUAL(std::string("application/binary"), header);
}

void
LuaTest::testResponseRedirect() {

    boost::shared_ptr<Request> request(new Request());
    boost::shared_ptr<Response> response(new Response());   
    boost::shared_ptr<State> state(new State());

    std::vector<std::string> env;
    env.push_back("QUERY_STRING=query=QUERY");
    env.push_back("HTTP_HOST=fireball.yandex.ru");
    env.push_back("HTTP_COOKIE=SessionId=2.12.85.0.6;");
    
    attachRequest(request.get(), env);

    boost::shared_ptr<RequestData> data(new RequestData(request, response, state));
    boost::shared_ptr<Script> script = ScriptFactory::createScript("lua-response-redirect.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));

    CPPUNIT_ASSERT_EQUAL((unsigned short)302, response->status());
    
    const HeaderMap& headers = response->outHeaders();
    HeaderMap::const_iterator it = headers.find("Location");
    std::string header = it == headers.end() ? StringUtils::EMPTY_STRING : it->second;    
    
    CPPUNIT_ASSERT_EQUAL(std::string("http://example.com/"), header);

}

void
LuaTest::testEncode() {
    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = ScriptFactory::createScript("lua-encode.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());
    CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "/page/lua"));
    CPPUNIT_ASSERT_EQUAL(
        std::string("%CF%F0%E5%E2%E5%E4\nПревед\n%D0%9F%D1%80%D0%B5%D0%B2%D0%B5%D0%B4\nПревед"),
        XmlUtils::xpathValue(doc.get(), "/page/lua", "Bye")
    );
}

void
LuaTest::testCookie() {

    boost::shared_ptr<Request> request(new Request());
    boost::shared_ptr<Response> response(new Response());   
    boost::shared_ptr<State> state(new State());
    boost::shared_ptr<RequestData> data(new RequestData(request, response, state));
    boost::shared_ptr<Script> script = ScriptFactory::createScript("lua-cookie.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));

    CPPUNIT_ASSERT(NULL != doc.get());
    
    const CookieSet& cookies = response->outCookies();
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
    CPPUNIT_ASSERT_EQUAL(time_t(Cookie::MAX_LIVE_TIME), cookie_it->expires());
}


void
LuaTest::testBadType() {
    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = ScriptFactory::createScript("lua-badtype.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());
    CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "/page/xscript_invoke_failed"));
}

void
LuaTest::testBadArgCount() {
    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = ScriptFactory::createScript("lua-badargcount.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());
    CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "/page/xscript_invoke_failed"));
}

void
LuaTest::testBadCode() {
    boost::shared_ptr<Script> script = ScriptFactory::createScript("lua-badcode.xml");
}


void
LuaTest::testMultiBlock() {
    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = ScriptFactory::createScript("lua-multi.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    State* state = data->state();
    CPPUNIT_ASSERT_EQUAL(state->asString("bar"), std::string("baz"));
}

void
LuaTest::testMD5() {
    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = ScriptFactory::createScript("lua-md5.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));

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

    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = ScriptFactory::createScript("lua-domain.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    State* state = ctx->state();

    CPPUNIT_ASSERT_EQUAL(std::string("hghltd.yandex.net"), state->asString("no_level"));
    CPPUNIT_ASSERT_EQUAL(std::string("net"), state->asString("tld"));
    CPPUNIT_ASSERT_EQUAL(std::string("www.yandex.ru"), state->asString("no_level_no_scheme"));
    CPPUNIT_ASSERT_EQUAL(std::string("yandex.ru"), state->asString("yandex.ru"));
    CPPUNIT_ASSERT_EQUAL(std::string("yandex.ru"), state->asString("no_scheme"));
    CPPUNIT_ASSERT_EQUAL(std::string("localhost"), state->asString("localhost"));
    CPPUNIT_ASSERT(!state->has("invalid"));
    CPPUNIT_ASSERT(!state->has("localfile"));
    CPPUNIT_ASSERT(!state->has("empty"));
}

void
LuaTest::testXmlEncode() {
    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = ScriptFactory::createScript("lua-xmlescape.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));

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
    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = ScriptFactory::createScript("lua-logger.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));

    CPPUNIT_ASSERT(NULL != doc.get());
}
