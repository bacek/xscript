#include "settings.h"

#include <stdexcept>
#include <boost/lexical_cast.hpp>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/util.h"
#include "xscript/state.h"
#include "xscript/script.h"
#include "xscript/request.h"
#include "xscript/context.h"
#include "xscript/response.h"
#include "xscript/request_data.h"
#include "xscript/request_impl.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

using namespace xscript;

class LuaTest : public CppUnit::TestFixture
{
public:
	
	void testPrint();

	void testState();
	void testRequest();
	void testResponse();
	void testResponseRedirect();
	
	void testEncode();
	void testCookie();

	void testBadCode();
	void testBadType();
	void testBadArgCount();

private:
	CPPUNIT_TEST_SUITE(LuaTest);
	CPPUNIT_TEST(testPrint);
	CPPUNIT_TEST(testState);
	CPPUNIT_TEST(testRequest);
	CPPUNIT_TEST(testResponse);
	CPPUNIT_TEST(testResponseRedirect);

	CPPUNIT_TEST(testEncode);
	CPPUNIT_TEST(testCookie);

	CPPUNIT_TEST(testBadType);
	CPPUNIT_TEST(testBadArgCount);
	CPPUNIT_TEST_EXCEPTION(testBadCode, std::runtime_error);
	CPPUNIT_TEST_SUITE_END();
};

#ifdef HAVE_LUA_BLOCK

CPPUNIT_TEST_SUITE_REGISTRATION( LuaTest );

#endif

void
LuaTest::testPrint() {
	
	using namespace xscript;
	
	RequestData data;
	boost::shared_ptr<Script> script = Script::create("lua-print.xml");
	boost::shared_ptr<Context> ctx(new Context(script, data));
	ContextStopper ctx_stopper(ctx);
	
	XmlDocHelper doc(script->invoke(ctx));
	CPPUNIT_ASSERT(NULL != doc.get());
	CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "/page/lua"));
	CPPUNIT_ASSERT_EQUAL(
		std::string("Hello\nWorld!\n"),
		XmlUtils::xpathValue(doc.get(), "/page/lua", "Bye")
	);
}


void
LuaTest::testState() {
	
	using namespace xscript;
	
	RequestData data;
	boost::shared_ptr<Script> script = Script::create("lua-state.xml");
	boost::shared_ptr<Context> ctx(new Context(script, data));
	ContextStopper ctx_stopper(ctx);
	
	XmlDocHelper doc(script->invoke(ctx));
	CPPUNIT_ASSERT(NULL != doc.get());
	
	boost::shared_ptr<State> state = data.state();
	for (int i = 1; i <= 10; ++i) {
		
		std::string num = boost::lexical_cast<std::string>(i);
		CPPUNIT_ASSERT_EQUAL(state->asString("long " + num), num);
		
		CPPUNIT_ASSERT_EQUAL(state->asString("string " + num), 
			boost::lexical_cast<std::string>(i * 2));
		CPPUNIT_ASSERT_EQUAL(state->asString("long long " + num),
			boost::lexical_cast<std::string>(i * 3));
	}
}

class FakeResponse : public xscript::Response
{
public:
	void setCookie(const xscript::Cookie &cookie) { cookies[cookie.name()] = cookie; };
	void setStatus(unsigned short s) { status = s; };
	void sendError(unsigned short s, const std::string& message) {};
	void setHeader(const std::string &name, const std::string &value) {
		headers[name] = value;
	};
	
	std::streamsize write(const char *buf, std::streamsize size) { return size; };
	std::string outputHeader(const std::string &name) const { return ""; };
	
	void sendHeaders() {};

	unsigned short status;
	std::string content_type;
	std::map<std::string, std::string> headers;
	std::map<std::string, Cookie> cookies;
};
	

void
LuaTest::testRequest() {
	RequestImpl request;
	FakeResponse response;
	boost::shared_ptr<State> state(new State());

	request.setArg("query", "QUERY");
	request.addInputHeader("Host", "fireball.yandex.ru");
	request.addInputCookie("SessionId", "2.12.85.0.6");


	RequestData data = RequestData(&request, &response, state);
	boost::shared_ptr<Script> script = Script::create("lua-request.xml");
	boost::shared_ptr<Context> ctx(new Context(script, data));
	ContextStopper ctx_stopper(ctx);
	
	XmlDocHelper doc(script->invoke(ctx));

	CPPUNIT_ASSERT("QUERY" == state->asString("test args"));
	CPPUNIT_ASSERT("fireball.yandex.ru" == state->asString("test headers"));
	CPPUNIT_ASSERT("2.12.85.0.6" == state->asString("test cookies"));

}

void
LuaTest::testResponse() {
	RequestImpl request;
	FakeResponse response;
	boost::shared_ptr<State> state(new State());

	request.setArg("query", "QUERY");
	request.addInputHeader("Host", "fireball.yandex.ru");
	request.addInputCookie("SessionId", "2.12.85.0.6");


	RequestData data = RequestData(&request, &response, state);
	boost::shared_ptr<Script> script = Script::create("lua-response.xml");
	boost::shared_ptr<Context> ctx(new Context(script, data));
	ContextStopper ctx_stopper(ctx);
	
	XmlDocHelper doc(script->invoke(ctx));

	CPPUNIT_ASSERT_EQUAL((unsigned short)404, response.status);
	CPPUNIT_ASSERT_EQUAL(std::string("Foo Bar"), response.headers["X-Header"]);
	CPPUNIT_ASSERT_EQUAL(std::string("application/binary"), response.headers["Content-type"]);
}

void
LuaTest::testResponseRedirect() {
	RequestImpl request;
	FakeResponse response;
	boost::shared_ptr<State> state(new State());

	request.setArg("query", "QUERY");
	request.addInputHeader("Host", "fireball.yandex.ru");
	request.addInputCookie("SessionId", "2.12.85.0.6");


	RequestData data = RequestData(&request, &response, state);
	boost::shared_ptr<Script> script = Script::create("lua-response-redirect.xml");
	boost::shared_ptr<Context> ctx(new Context(script, data));
	ContextStopper ctx_stopper(ctx);
	
	XmlDocHelper doc(script->invoke(ctx));

	CPPUNIT_ASSERT_EQUAL((unsigned short)302, response.status);
	CPPUNIT_ASSERT_EQUAL(std::string("http://example.com/"), response.headers["Location"]);

}

void
LuaTest::testEncode() {
	
	using namespace xscript;
	
	RequestData data;
	boost::shared_ptr<Script> script = Script::create("lua-encode.xml");
	boost::shared_ptr<Context> ctx(new Context(script, data));
	ContextStopper ctx_stopper(ctx);
	
	XmlDocHelper doc(script->invoke(ctx));
	CPPUNIT_ASSERT(NULL != doc.get());
	CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "/page/lua"));
	CPPUNIT_ASSERT_EQUAL(
		std::string("%CF%F0%E5%E2%E5%E4\nПревед\n"),
		XmlUtils::xpathValue(doc.get(), "/page/lua", "Bye")
	);
}

void
LuaTest::testCookie() {
	RequestImpl request;
	FakeResponse response;
	boost::shared_ptr<State> state(new State());


	RequestData data = RequestData(&request, &response, state);
	boost::shared_ptr<Script> script = Script::create("lua-cookie.xml");
	boost::shared_ptr<Context> ctx(new Context(script, data));
	ContextStopper ctx_stopper(ctx);
	
	XmlDocHelper doc(script->invoke(ctx));

	CPPUNIT_ASSERT(NULL != doc.get());
	CPPUNIT_ASSERT(response.cookies.end() != response.cookies.find("foo"));
	Cookie c = response.cookies["foo"];
	CPPUNIT_ASSERT_EQUAL(std::string(".example.com"), c.path());

	CPPUNIT_ASSERT(
		XmlUtils::xpathValue(doc.get(), "/page/lua", "Bye").find(
			".example.com"
		) != std::string::npos
	);
}


void
LuaTest::testBadType() {

	using namespace xscript;
	
	RequestData data;
	boost::shared_ptr<Script> script = Script::create("lua-badtype.xml");
	boost::shared_ptr<Context> ctx(new Context(script, data));
	ContextStopper ctx_stopper(ctx);
	
	XmlDocHelper doc(script->invoke(ctx));
	CPPUNIT_ASSERT(NULL != doc.get());
	CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "/page/error"));
}

void
LuaTest::testBadArgCount() {

	using namespace xscript;
	
	RequestData data;
	boost::shared_ptr<Script> script = Script::create("lua-badargcount.xml");
	boost::shared_ptr<Context> ctx(new Context(script, data));
	ContextStopper ctx_stopper(ctx);
	
	XmlDocHelper doc(script->invoke(ctx));
	CPPUNIT_ASSERT(NULL != doc.get());
	CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "/page/error"));
}

void
LuaTest::testBadCode() {
	using namespace xscript;
	boost::shared_ptr<Script> script = Script::create("lua-badcode.xml");
}
