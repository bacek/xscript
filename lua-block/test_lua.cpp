#include "settings.h"

#include <stdexcept>
#include <boost/lexical_cast.hpp>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/xml_util.h"
#include "xscript/state.h"
#include "xscript/script.h"
#include "xscript/request.h"
#include "xscript/context.h"
#include "xscript/request_data.h"
#include "xscript/request_impl.h"
#include "internal/default_request_response.h"

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

    CPPUNIT_TEST(testLogger);

    CPPUNIT_TEST_SUITE_END();
};

#ifdef HAVE_LUA_BLOCK

CPPUNIT_TEST_SUITE_REGISTRATION( LuaTest );

#endif

void
LuaTest::testPrint() {
    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = Script::create("lua-print.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));

    CPPUNIT_ASSERT(NULL != doc.get());
    CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "/page/lua"));
    CPPUNIT_ASSERT_EQUAL(
        std::string("Hello\n&nbsp;\nWorld!\n"),
        XmlUtils::xpathValue(doc.get(), "/page/lua", "Bye")
    );
}


void
LuaTest::testState() {
    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = Script::create("lua-state.xml");
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
    boost::shared_ptr<Script> script = Script::create("lua-state-has.xml");
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
    boost::shared_ptr<Script> script = Script::create("lua-state-is.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    State* state = data->state();

    CPPUNIT_ASSERT(state->asBool("guard0_passed"));
    CPPUNIT_ASSERT(state->asBool("guard1_passed"));
    CPPUNIT_ASSERT(state->asBool("guard2_passed"));
}


class FakeRequestResponse : public DefaultRequestResponse {

public:
    void setCookie(const xscript::Cookie &cookie) {
        cookies[cookie.name()] = cookie;
    };
    void setStatus(unsigned short s) {
        status = s;
    };
    void setHeader(const std::string &name, const std::string &value) {
        headers[name] = value;
    };

    std::streamsize write(const char *, std::streamsize size) {
        return size;
    };

    void setIsSecure(bool is) {
        is_secure = is;
    }

    bool isSecure() const {
        return is_secure;
    }

    void setContentLength(std::streamsize length) {
        content_length = length;
    }

    std::streamsize getContentLength() const {
        return content_length;
    }

    unsigned short status;
    std::string content_type;
    std::streamsize content_length;
    std::map<std::string, std::string> headers;
    std::map<std::string, Cookie> cookies;

    bool is_secure;
};

void
LuaTest::testRequest() {

    boost::shared_ptr<RequestResponse> request(new FakeRequestResponse());
    FakeRequestResponse* fake_request = dynamic_cast<FakeRequestResponse*>(request.get());
    boost::shared_ptr<State> state(new State());

    fake_request->setArg("query", "QUERY");
    fake_request->addInputHeader("Host", "fireball.yandex.ru");
    fake_request->addInputCookie("SessionId", "2.12.85.0.6");
    fake_request->setIsSecure(true);
    fake_request->setContentLength(42);

    boost::shared_ptr<RequestData> data(new RequestData(request, state));
    boost::shared_ptr<Script> script = Script::create("lua-request.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));

    CPPUNIT_ASSERT_EQUAL(std::string("QUERY"), state->asString("test args"));
    CPPUNIT_ASSERT_EQUAL(std::string("fireball.yandex.ru"), state->asString("test headers"));
    CPPUNIT_ASSERT_EQUAL(std::string("2.12.85.0.6"), state->asString("test cookies"));
    CPPUNIT_ASSERT_EQUAL(true, state->asBool("test isSecure"));
    CPPUNIT_ASSERT_EQUAL(42LL, state->asLongLong("test content_length"));
}

void
LuaTest::testResponse() {

    boost::shared_ptr<RequestResponse> request(new FakeRequestResponse());
    FakeRequestResponse* fake_request = dynamic_cast<FakeRequestResponse*>(request.get());
    boost::shared_ptr<State> state(new State());

    fake_request->setArg("query", "QUERY");
    fake_request->addInputHeader("Host", "fireball.yandex.ru");
    fake_request->addInputCookie("SessionId", "2.12.85.0.6");

    boost::shared_ptr<RequestData> data(new RequestData(request, state));
    boost::shared_ptr<Script> script = Script::create("lua-response.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));

    CPPUNIT_ASSERT_EQUAL((unsigned short)404, fake_request->status);
    CPPUNIT_ASSERT_EQUAL(std::string("Foo Bar"), fake_request->headers["X-Header"]);
    CPPUNIT_ASSERT_EQUAL(std::string("application/binary"), fake_request->headers["Content-type"]);
}

void
LuaTest::testResponseRedirect() {

    boost::shared_ptr<RequestResponse> request(new FakeRequestResponse());
    FakeRequestResponse* fake_request = dynamic_cast<FakeRequestResponse*>(request.get());
    boost::shared_ptr<State> state(new State());

    fake_request->setArg("query", "QUERY");
    fake_request->addInputHeader("Host", "fireball.yandex.ru");
    fake_request->addInputCookie("SessionId", "2.12.85.0.6");

    boost::shared_ptr<RequestData> data(new RequestData(request, state));
    boost::shared_ptr<Script> script = Script::create("lua-response-redirect.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));

    CPPUNIT_ASSERT_EQUAL((unsigned short)302, fake_request->status);
    CPPUNIT_ASSERT_EQUAL(std::string("http://example.com/"), fake_request->headers["Location"]);

}

void
LuaTest::testEncode() {
    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = Script::create("lua-encode.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());
    CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "/page/lua"));
    CPPUNIT_ASSERT_EQUAL(
        std::string("%CF%F0%E5%E2%E5%E4\nПревед\n%D0%9F%D1%80%D0%B5%D0%B2%D0%B5%D0%B4\nПревед\n"),
        XmlUtils::xpathValue(doc.get(), "/page/lua", "Bye")
    );
}

void
LuaTest::testCookie() {

    boost::shared_ptr<RequestResponse> request(new FakeRequestResponse());
    FakeRequestResponse* fake_request = dynamic_cast<FakeRequestResponse*>(request.get());
    boost::shared_ptr<State> state(new State());

    boost::shared_ptr<RequestData> data(new RequestData(request, state));
    boost::shared_ptr<Script> script = Script::create("lua-cookie.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));

    CPPUNIT_ASSERT(NULL != doc.get());
    CPPUNIT_ASSERT(fake_request->cookies.end() != fake_request->cookies.find("foo"));
    Cookie c = fake_request->cookies["foo"];
    CPPUNIT_ASSERT_EQUAL(std::string("/some/path"), c.path());
    CPPUNIT_ASSERT_EQUAL(std::string(".example.com"), c.domain());
    CPPUNIT_ASSERT_EQUAL(time_t(123456789), c.expires());

    std::string out = XmlUtils::xpathValue(doc.get(), "/page/lua", "Bye");
    CPPUNIT_ASSERT(out.find(".example.com") != std::string::npos);
    CPPUNIT_ASSERT(out.find( "/some/path") != std::string::npos);
    CPPUNIT_ASSERT(out.find( "123456789") != std::string::npos);

    Cookie c2 = fake_request->cookies["baz"];
    CPPUNIT_ASSERT_EQUAL(time_t(Cookie::MAX_LIVE_TIME), c2.expires());
}


void
LuaTest::testBadType() {
    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = Script::create("lua-badtype.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());
    CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "/page/xscript_invoke_failed"));
}

void
LuaTest::testBadArgCount() {
    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = Script::create("lua-badargcount.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());
    CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "/page/xscript_invoke_failed"));
}

void
LuaTest::testBadCode() {
    boost::shared_ptr<Script> script = Script::create("lua-badcode.xml");
}


void
LuaTest::testMultiBlock() {
    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = Script::create("lua-multi.xml");
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
    boost::shared_ptr<Script> script = Script::create("lua-md5.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));

    CPPUNIT_ASSERT(NULL != doc.get());
    CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "/page/lua"));
    CPPUNIT_ASSERT_EQUAL(
        std::string("5946210c9e93ae37891dfe96c3e39614\n"),
        XmlUtils::xpathValue(doc.get(), "/page/lua", "foo")
    );
}

// Incomplete test for logger... Just check manually default.log
void
LuaTest::testLogger() {
    boost::shared_ptr<RequestData> data(new RequestData());
    boost::shared_ptr<Script> script = Script::create("lua-logger.xml");
    boost::shared_ptr<Context> ctx(new Context(script, data));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));

    CPPUNIT_ASSERT(NULL != doc.get());
}
