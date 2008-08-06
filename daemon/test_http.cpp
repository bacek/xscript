#include "settings.h"

#include <fstream>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "server_request.h"
#include "xscript/state.h"
#include "xscript/script.h"
#include "xscript/context.h"
#include "xscript/request_data.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

class HttpTest : public CppUnit::TestFixture {
public:
    void testGetByState();
    void testGetByRequest();

private:
    CPPUNIT_TEST_SUITE(HttpTest);
    CPPUNIT_TEST(testGetByState);
    CPPUNIT_TEST(testGetByRequest);
    CPPUNIT_TEST_SUITE_END();

    static bool checkExists(const char* where, const char* what);
};

#ifdef HAVE_HTTP_BLOCK

CPPUNIT_TEST_SUITE_REGISTRATION(HttpTest);

#endif

bool
HttpTest::checkExists(const char* where, const char* what) {

    std::string buf;
    std::ifstream f(where);

    f.exceptions(std::ios::badbit);
    buf.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());

    return buf.find(what) != std::string::npos;
}

void
HttpTest::testGetByRequest() {

    using namespace xscript;

    char *env[] = { "REQUEST_METHOD=GET", "QUERY_STRING=text=moscow", "HTTP_HOST=help.yandex.ru", NULL };

    ServerRequest req;
    std::stringstream in, out;
    req.attach(&in, &out, env);

    boost::shared_ptr<State> state(new State());

    CPPUNIT_ASSERT_EQUAL(std::string("GET"), req.getRequestMethod());
    CPPUNIT_ASSERT_EQUAL(std::string("help.yandex.ru"), req.getHeader("Host"));
    CPPUNIT_ASSERT_EQUAL(std::string("text=moscow"), req.getQueryString());

    CPPUNIT_ASSERT_EQUAL(std::string("moscow"), req.getArg("text"));

    boost::shared_ptr<Script> script = Script::create("http-get-by-request.xml");
    boost::shared_ptr<Context> ctx(new Context(script, RequestData(&req, &req, state)));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    xmlSaveFile("http-results-request.xml", doc.get());
    CPPUNIT_ASSERT(checkExists("http-results-request.xml", "<text>moscow</text>"));
}

void
HttpTest::testGetByState() {

    using namespace xscript;

    char *env[] = { "REQUEST_METHOD=GET", "HTTP_HOST=help.yandex.ru", NULL };

    ServerRequest req;
    std::stringstream in, out;
    req.attach(&in, &out, env);

    CPPUNIT_ASSERT_EQUAL(std::string("GET"), req.getRequestMethod());
    CPPUNIT_ASSERT_EQUAL(std::string("help.yandex.ru"), req.getHeader("Host"));

    boost::shared_ptr<State> state(new State());
    state->setString("text", "moscow");

    boost::shared_ptr<Script> script = Script::create("http-get-by-state.xml");
    boost::shared_ptr<Context> ctx(new Context(script, RequestData(&req, &req, state)));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());

    xmlSaveFile("http-results-state.xml", doc.get());
    CPPUNIT_ASSERT(checkExists("http-results-state.xml", "<text>moscow</text>"));
}
