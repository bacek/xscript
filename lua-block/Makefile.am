pkglib_LTLIBRARIES = xscript-lua.la

xscript_lua_la_SOURCES = exception.cpp lua_block.cpp request_methods.cpp \
	response_methods.cpp stack.cpp state_methods.cpp cookie_methods.cpp

xscript_lua_la_LIBADD = ../library/libxscript.la
xscript_lua_la_LDFLAGS = @lua_LIBS@ -module -avoid-version

noinst_HEADERS = exception.h lua_block.h method_map.h request_methods.h \
	response_methods.h stack.h state_methods.h

AM_CPPFLAGS = -I@top_srcdir@/include -I@builddir@/config -D_REENTRANT @lua_CFLAGS@
AM_CXXFLAGS = -Wall -fexceptions -frtti -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = -lboost_thread -lboost_regex -lboost_filesystem

if HAVE_TESTS

TESTS = test
check_PROGRAMS = test

test_SOURCES = test_main.cpp test_lua.cpp

test_LDADD = ../library/libxscript.la 
test_LDFLAGS = -ldl -lcppunit -export-dynamic

endif
