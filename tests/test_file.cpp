#include "settings.h"

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/context.h"
#include "xscript/script.h"
#include "xscript/script_factory.h"
#include "xscript/state.h"
#include "xscript/xml_util.h"

#include "../file-block/file_block.h"
#include "../file-block/file_extension.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

using namespace xscript;

class FileTest : public CppUnit::TestFixture {
public:
    CPPUNIT_TEST_SUITE(FileTest);
    CPPUNIT_TEST_EXCEPTION(testUnknowMethod, std::invalid_argument);
    CPPUNIT_TEST(testLoad);
    CPPUNIT_TEST(testInclude);
    CPPUNIT_TEST(testTag);
    CPPUNIT_TEST_SUITE_END();

public:
    void testUnknowMethod();
    void testLoad();
    void testInclude();
    void testTag();
};

#ifdef HAVE_FILE_BLOCK

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(FileTest, "file");
CPPUNIT_REGISTRY_ADD("file", "xscript");

#endif

void
FileTest::testUnknowMethod() {
    boost::shared_ptr<Script> script = ScriptFactory::createScript("./file-unknownMethod.xml");
}


void
FileTest::testLoad() {

    boost::shared_ptr<Request> request(new Request());
    boost::shared_ptr<Response> response(new Response());
    boost::shared_ptr<State> state(new State());
    boost::shared_ptr<Script> script = ScriptFactory::createScript("./file-load.xml");
    boost::shared_ptr<Context> ctx(new Context(script, state, request, response));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());
    CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "//include-data"));
}

void
FileTest::testInclude() {

    boost::shared_ptr<Request> request(new Request());
    boost::shared_ptr<Response> response(new Response());
    boost::shared_ptr<State> state(new State());
    boost::shared_ptr<Script> script = ScriptFactory::createScript("./file-include.xml");
    boost::shared_ptr<Context> ctx(new Context(script, state, request, response));
    ContextStopper ctx_stopper(ctx);

    XmlDocHelper doc(script->invoke(ctx));
    CPPUNIT_ASSERT(NULL != doc.get());
    CPPUNIT_ASSERT(XmlUtils::xpathExists(doc.get(), "//include-data"));
}

void
FileTest::testTag() {
    /*
     * There is no way to check tag from outside...
     *
    XmlNodeHelper blockNode(xmlNewNode(0, (const xmlChar*) "file"));
    xmlNewTextChild(blockNode.get(), 0, (const xmlChar*) "method", (const xmlChar*) "load");
    xmlNewTextChild(blockNode.get(), 0, (const xmlChar*) "param", (const xmlChar*) "include.xml");

    xmlNodePtr tagNode = xmlNewChild(blockNode.get(), 0, (const xmlChar*) "param", 0);
    xmlNewProp(tagNode, (const xmlChar*) "type", (const xmlChar*) "Tag");

    boost::shared_ptr<Script> script = ScriptFactory::createScript("./file-load.xml");

    FileBlock fileBlock(FileExtension::instance(), script.get(), blockNode.get());
    */
}


