#include "settings.h"

#include <sstream>
#include <exception>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "xscript/state.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

class StateTest : public CppUnit::TestFixture {
public:
    void testBool();
    void testLong();
    void testString();
    void testDouble();
    void testLongLong();
    void testClearAll();
    void testClearPrefix();
    void testBadCast();
    void testNonexistent();

    void testIs();

private:
    CPPUNIT_TEST_SUITE(StateTest);
    CPPUNIT_TEST(testBool);
    CPPUNIT_TEST(testLong);
    CPPUNIT_TEST(testString);
    CPPUNIT_TEST(testDouble);
    CPPUNIT_TEST(testLongLong);
    CPPUNIT_TEST(testClearAll);
    CPPUNIT_TEST(testClearPrefix);
    CPPUNIT_TEST_EXCEPTION(testBadCast, std::exception);
    CPPUNIT_TEST_EXCEPTION(testNonexistent, std::exception);
    CPPUNIT_TEST(testIs);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StateTest, "state");
CPPUNIT_REGISTRY_ADD("state", "xscript");

void
StateTest::testBool() {

    using namespace xscript;
    std::auto_ptr<State> state(new State());
    CPPUNIT_ASSERT(state.get());

    state->setBool("true_key", true);
    CPPUNIT_ASSERT(state->has("true_key"));
    CPPUNIT_ASSERT_EQUAL(true, state->asBool("true_key"));
    CPPUNIT_ASSERT_EQUAL(static_cast<boost::int32_t>(1), state->asLong("true_key"));
    CPPUNIT_ASSERT_EQUAL(static_cast<boost::int64_t>(1), state->asLongLong("true_key"));
    CPPUNIT_ASSERT_EQUAL(std::string("1"), state->asString("true_key"));

    state->setBool("false_key", false);
    CPPUNIT_ASSERT(state->has("false_key"));
    CPPUNIT_ASSERT_EQUAL(false, state->asBool("false_key"));
    CPPUNIT_ASSERT_EQUAL(static_cast<boost::int32_t>(0), state->asLong("false_key"));
    CPPUNIT_ASSERT_EQUAL(static_cast<boost::int64_t>(0), state->asLongLong("false_key"));
    CPPUNIT_ASSERT_EQUAL(std::string("0"), state->asString("false_key"));

}

void
StateTest::testLong() {

    using namespace xscript;
    std::auto_ptr<State> state(new State());
    CPPUNIT_ASSERT(state.get());

    state->setLong("long_key", 15);
    CPPUNIT_ASSERT(state->has("long_key"));
    CPPUNIT_ASSERT_EQUAL(true, state->asBool("long_key"));
    CPPUNIT_ASSERT_EQUAL(static_cast<boost::int32_t>(15), state->asLong("long_key"));
    CPPUNIT_ASSERT_EQUAL(std::string("15"), state->asString("long_key"));

}

void
StateTest::testString() {

    using namespace xscript;
    std::auto_ptr<State> state(new State());
    CPPUNIT_ASSERT(state.get());

    state->setString("string_key", "test");
    CPPUNIT_ASSERT(state->has("string_key"));
    CPPUNIT_ASSERT_EQUAL(true, state->asBool("string_key"));
    CPPUNIT_ASSERT_EQUAL(std::string("test"), state->asString("string_key"));

    state->setString("empty_key", "");
    CPPUNIT_ASSERT(state->has("empty_key"));
    CPPUNIT_ASSERT_EQUAL(false, state->asBool("empty_key"));
    CPPUNIT_ASSERT_EQUAL(static_cast<boost::int32_t>(0), state->asLong("empty_key"));
    CPPUNIT_ASSERT_EQUAL(static_cast<boost::int64_t>(0), state->asLongLong("empty_key"));
    CPPUNIT_ASSERT_EQUAL(std::string(""), state->asString("empty_key"));
}

void
StateTest::testDouble() {

    using namespace xscript;
    std::auto_ptr<State> state(new State());
    CPPUNIT_ASSERT(state.get());

    state->setDouble("double_key", 12);
    CPPUNIT_ASSERT(state->has("double_key"));
    CPPUNIT_ASSERT_EQUAL(true, state->asBool("double_key"));
    CPPUNIT_ASSERT_EQUAL(static_cast<double>(12), state->asDouble("double_key"));
    CPPUNIT_ASSERT_EQUAL(std::string("12"), state->asString("double_key"));
    CPPUNIT_ASSERT_EQUAL(static_cast<boost::int32_t>(12), state->asLong("double_key"));
}

void
StateTest::testLongLong() {

    using namespace xscript;
    std::auto_ptr<State> state(new State());
    CPPUNIT_ASSERT(state.get());

    state->setLongLong("longlong_key", 15);
    CPPUNIT_ASSERT_EQUAL(true, state->asBool("longlong_key"));
    CPPUNIT_ASSERT_EQUAL(static_cast<boost::int64_t>(15), state->asLongLong("longlong_key"));
    CPPUNIT_ASSERT_EQUAL(std::string("15"), state->asString("longlong_key"));
}

void
StateTest::testClearAll() {

    using namespace xscript;

    std::vector<std::string> v;
    std::auto_ptr<State> state(new State());
    CPPUNIT_ASSERT(state.get());

    for (int i = 0; i < 10; ++i) {
        std::stringstream stream;
        stream << "test-key-" << i;
        std::string s = stream.str();
        state->setString(s, s);
    }

    state->keys(v);
    CPPUNIT_ASSERT_EQUAL(static_cast<std::vector<std::string>::size_type>(10), v.size());

    state->clear();

    state->keys(v);
    CPPUNIT_ASSERT_EQUAL(static_cast<std::vector<std::string>::size_type>(0), v.size());
}

void
StateTest::testClearPrefix() {

    using namespace xscript;

    std::vector<std::string> v;
    std::auto_ptr<State> state(new State());
    CPPUNIT_ASSERT(state.get());

    for (int i = 0; i < 10; ++i) {
        std::stringstream stream;
        stream << "test-key-" << i;
        std::string s = stream.str();
        state->setString(s, s);
    }

    state->setLong("long-test-1", 1);
    state->setLong("long-test-2", 2);

    state->erasePrefix("test");
    state->keys(v);

    CPPUNIT_ASSERT_EQUAL(static_cast<std::vector<std::string>::size_type>(2), v.size());
}

void
StateTest::testBadCast() {

    using namespace xscript;
    std::auto_ptr<State> state(new State());
    CPPUNIT_ASSERT(state.get());

    state->setString("string_key", "test_string");
    state->asDouble("string_key");
}

void
StateTest::testNonexistent() {

    using namespace xscript;
    std::auto_ptr<State> state(new State());
    CPPUNIT_ASSERT(state.get());

    state->asString("nonexistent");
}


void
StateTest::testIs() {

    using namespace xscript;
    std::auto_ptr<State> state(new State());
    CPPUNIT_ASSERT(state.get());

    // Non existed State evaluated to false
    CPPUNIT_ASSERT_EQUAL(false, state->is("guard"));

    // Boolean state evaluated to self.
    state->setBool("guard", false);
    CPPUNIT_ASSERT_EQUAL(false, state->is("guard"));
    state->setBool("guard", true);
    CPPUNIT_ASSERT_EQUAL(true, state->is("guard"));

    // Empty string evaluated to false.
    state->setString("guard", "");
    CPPUNIT_ASSERT_EQUAL(false, state->is("guard"));
    // Non empty string evaluated to true.
    state->setString("guard", "foo");
    CPPUNIT_ASSERT_EQUAL(true, state->is("guard"));

    // Sanity check. "0" is true. A bit confusing, heh...
    state->setString("guard", "0");
    CPPUNIT_ASSERT_EQUAL(true, state->is("guard"));
   
    // Zero numeric evaluated to false
    state->setLong("guard", 0);
    CPPUNIT_ASSERT_EQUAL(false, state->is("guard"));
    // Non zero to true
    state->setLong("guard", 42);
    CPPUNIT_ASSERT_EQUAL(true, state->is("guard"));
}

