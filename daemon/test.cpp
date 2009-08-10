#include "settings.h"

#include <sstream>
#include <fstream>
#include <cppunit/TestFixture.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include "server_request.h"
#include "xscript/config.h"

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
    CPPUNIT_TEST(testMultipart);
    CPPUNIT_TEST(testStatusString);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(RequestTest);

void
RequestTest::testGet() {

    using namespace xscript;

    char *env[] = { "REQUEST_METHOD=GET", "QUERY_STRING=test=pass&success=try%20again", "HTTP_HOST=yandex.ru", NULL };

    ServerRequest req;
    std::stringstream in, out;
    req.attach(&in, &out, env);

    CPPUNIT_ASSERT_EQUAL(std::string("GET"), req.getRequestMethod());

    CPPUNIT_ASSERT_EQUAL(std::string("yandex.ru"), req.getHeader("Host"));
    CPPUNIT_ASSERT_EQUAL(std::string("test=pass&success=try%20again"), req.getQueryString());

    CPPUNIT_ASSERT_EQUAL(std::string("pass"), req.getArg("test"));
    CPPUNIT_ASSERT_EQUAL(std::string("try again"), req.getArg("success"));
}

void
RequestTest::testPost() {

    using namespace xscript;

    char *env[] = { "REQUEST_METHOD=POST", "HTTP_CONTENT_LENGTH=29", "HTTP_HOST=yandex.ru", NULL };

    ServerRequest req;
    std::stringstream in("test=pass&success=try%20again"), out;
    req.attach(&in, &out, env);

    CPPUNIT_ASSERT_EQUAL(std::string("POST"), req.getRequestMethod());

    CPPUNIT_ASSERT_EQUAL(std::string("yandex.ru"), req.getHeader("Host"));
    CPPUNIT_ASSERT_EQUAL(std::string(""), req.getQueryString());

    CPPUNIT_ASSERT_EQUAL(std::string("pass"), req.getArg("test"));
    CPPUNIT_ASSERT_EQUAL(std::string("try again"), req.getArg("success"));

}

void
RequestTest::testCookie() {

    using namespace xscript;

    char *env[] = { "REQUEST_METHOD=GET", "QUERY_STRING=test=pass&success=try%20again", "HTTP_HOST=yandex.ru",
                    "HTTP_COOKIE=yandexuid=921562781154947430; yandex_login=highpower; my=Yx4CAAA", NULL
                  };

    ServerRequest req;
    std::stringstream in, out;
    req.attach(&in, &out, env);

    CPPUNIT_ASSERT_EQUAL(std::string("GET"), req.getRequestMethod());

    CPPUNIT_ASSERT_EQUAL(std::string("yandex.ru"), req.getHeader("Host"));
    CPPUNIT_ASSERT_EQUAL(std::string("test=pass&success=try%20again"), req.getQueryString());

    CPPUNIT_ASSERT_EQUAL(std::string("pass"), req.getArg("test"));
    CPPUNIT_ASSERT_EQUAL(std::string("try again"), req.getArg("success"));

    CPPUNIT_ASSERT_EQUAL(std::string("Yx4CAAA"), req.getCookie("my"));
    CPPUNIT_ASSERT_EQUAL(std::string("highpower"), req.getCookie("yandex_login"));
    CPPUNIT_ASSERT_EQUAL(std::string("921562781154947430"), req.getCookie("yandexuid"));

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

    ServerRequest req;

    char *env[] = { "REQUEST_METHOD=POST", "HTTP_HOST=yandex.ru", "HTTP_CONTENT_LENGTH=1361",
                    "CONTENT_TYPE=multipart/form-data; boundary=---------------------------15403834263040891721303455736", NULL
                  };

    std::fstream f("multipart-test.dat");
    std::stringstream out;

    req.attach(&f, &out, env);

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

    CPPUNIT_ASSERT_EQUAL(std::string("OK"), std::string(Parser::statusToString(200)));
    CPPUNIT_ASSERT_EQUAL(std::string("Moved Temporarily"), std::string(Parser::statusToString(302)));
    CPPUNIT_ASSERT_EQUAL(std::string("Not found"), std::string(Parser::statusToString(404)));
    CPPUNIT_ASSERT_EQUAL(std::string("Method not allowed"), std::string(Parser::statusToString(405)));
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
