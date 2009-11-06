#include "settings.h"

#include <sstream>
#include <fstream>
#include <cppunit/TestFixture.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/config.h"
#include "xscript/http_utils.h"
#include "xscript/request.h"

#include "internal/parser.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

class RequestTest : public CppUnit::TestFixture {
public:
    void testGet();
    void testPost();
    void testCookie();
    void testBoundary();
    void testMultipart();
    void testStatusString();

private:
    CPPUNIT_TEST_SUITE(RequestTest);
    CPPUNIT_TEST(testGet);
    CPPUNIT_TEST(testPost);
    CPPUNIT_TEST(testCookie);
    CPPUNIT_TEST(testBoundary);
//    CPPUNIT_TEST(testMultipart);
    CPPUNIT_TEST(testStatusString);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(RequestTest);

void
RequestTest::testGet() {

    using namespace xscript;

    char *env[] = {
        (char*)"REQUEST_METHOD=GET",
        (char*)"QUERY_STRING=test=pass&success=try%20again",
        (char*)"HTTP_HOST=yandex.ru",
        (char*)NULL
    };

    std::stringstream in;
    Request req;
    req.attach(&in, env);

    CPPUNIT_ASSERT_EQUAL(std::string("GET"), req.getRequestMethod());

    CPPUNIT_ASSERT_EQUAL(std::string("yandex.ru"), req.getHeader("Host"));
    CPPUNIT_ASSERT_EQUAL(std::string("test=pass&success=try%20again"), req.getQueryString());

    CPPUNIT_ASSERT_EQUAL(std::string("pass"), req.getArg("test"));
    CPPUNIT_ASSERT_EQUAL(std::string("try again"), req.getArg("success"));
}

void
RequestTest::testPost() {

    using namespace xscript;

    char *env[] = {
        (char*)"REQUEST_METHOD=POST",
        (char*)"HTTP_CONTENT_LENGTH=29",
        (char*)"HTTP_HOST=yandex.ru",
        (char*)NULL
    };

    Request req;
    std::stringstream in("test=pass&success=try%20again"), out;
    req.attach(&in, env);

    CPPUNIT_ASSERT_EQUAL(std::string("POST"), req.getRequestMethod());

    CPPUNIT_ASSERT_EQUAL(std::string("yandex.ru"), req.getHeader("Host"));
    CPPUNIT_ASSERT_EQUAL(std::string(""), req.getQueryString());

    CPPUNIT_ASSERT_EQUAL(std::string("pass"), req.getArg("test"));
    CPPUNIT_ASSERT_EQUAL(std::string("try again"), req.getArg("success"));
}

void
RequestTest::testCookie() {

    using namespace xscript;

    char *env[] = {
        (char*)"REQUEST_METHOD=GET",
        (char*)"QUERY_STRING=test=pass&success=try%20again",
        (char*)"HTTP_HOST=yandex.ru",
        (char*)"HTTP_COOKIE=cookie1=921562781154947430; cookie2=highpower; cookie3=Ddgfgdd",
        (char*)NULL
    };

    Request req;
    std::stringstream in;
    req.attach(&in, env);

    CPPUNIT_ASSERT_EQUAL(std::string("GET"), req.getRequestMethod());

    CPPUNIT_ASSERT_EQUAL(std::string("yandex.ru"), req.getHeader("Host"));
    CPPUNIT_ASSERT_EQUAL(std::string("test=pass&success=try%20again"), req.getQueryString());

    CPPUNIT_ASSERT_EQUAL(std::string("pass"), req.getArg("test"));
    CPPUNIT_ASSERT_EQUAL(std::string("try again"), req.getArg("success"));

    CPPUNIT_ASSERT_EQUAL(std::string("Ddgfgdd"), req.getCookie("cookie3"));
    CPPUNIT_ASSERT_EQUAL(std::string("highpower"), req.getCookie("cookie2"));
    CPPUNIT_ASSERT_EQUAL(std::string("921562781154947430"), req.getCookie("cookie1"));

}

void
RequestTest::testBoundary() {

    using namespace xscript;

    Range range = createRange("multipart/form-data; boundary=---------------------------7d71d92e31254");
    CPPUNIT_ASSERT_EQUAL(std::string("-----------------------------7d71d92e31254"),
                         Parser::getBoundary(range));
}

void
RequestTest::testMultipart() {

    using namespace xscript;

    char *env[] = {
        (char*)"REQUEST_METHOD=POST",
        (char*)"HTTP_HOST=yandex.ru",
        (char*)"HTTP_CONTENT_LENGTH=1361",
        (char*)"CONTENT_TYPE=multipart/form-data; boundary=---------------------------15403834263040891721303455736",
        (char*)NULL
    };

    std::fstream f("multipart-test.dat");

    Request req;
    req.attach(&f, env);

    CPPUNIT_ASSERT_EQUAL(std::string("test"), req.getArg("username"));
    CPPUNIT_ASSERT_EQUAL(std::string("test"), req.getArg("filename"));

    CPPUNIT_ASSERT_EQUAL(std::string("POST"), req.getRequestMethod());
    CPPUNIT_ASSERT_EQUAL(std::string("yandex.ru"), req.getHeader("Host"));

    CPPUNIT_ASSERT_EQUAL(true, req.hasFile("uploaded"));
    std::pair<const char*, std::streamsize> file = req.remoteFile("uploaded");
    CPPUNIT_ASSERT_EQUAL(static_cast<std::streamsize>(887), file.second);
}

void
RequestTest::testStatusString() {

    using namespace xscript;

    CPPUNIT_ASSERT_EQUAL(std::string("OK"), std::string(HttpUtils::statusToString(200)));
    CPPUNIT_ASSERT_EQUAL(std::string("Found"), std::string(HttpUtils::statusToString(302)));
    CPPUNIT_ASSERT_EQUAL(std::string("Not Found"), std::string(HttpUtils::statusToString(404)));
    CPPUNIT_ASSERT_EQUAL(std::string("Method Not Allowed"), std::string(HttpUtils::statusToString(405)));
}

int
main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    using namespace xscript;

    std::auto_ptr<Config> config =  Config::create("test.conf");
    config->startup();

    CppUnit::TextUi::TestRunner r;
    r.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());
    return r.run("", false) ? 0 : 1;
}
