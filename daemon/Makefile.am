bin_PROGRAMS = xscript-bin

xscript_bin_SOURCES = fcgi_server.cpp main.cpp uptime_counter.cpp

xscript_bin_LDADD = ../library/libxscript.la
xscript_bin_LDFLAGS = -lfcgi -lfcgi++ -export-dynamic

noinst_HEADERS = fcgi_server.h uptime_counter.h

dist_sysconf_DATA = xscript.conf.example xscript-ulimits.conf

AM_CPPFLAGS = -I@top_srcdir@/include -I@builddir@/config -D_REENTRANT @yandex_platform_CFLAGS@
AM_CXXFLAGS = -W -Wall -fexceptions -frtti -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = -lboost_thread -lboost_filesystem @yandex_platform_LIBS@

if HAVE_TESTS

TESTS = test
check_PROGRAMS = test

test_SOURCES = test.cpp

test_LDADD = ../library/libxscript.la
test_LDFLAGS = @CPPUNIT_LIBS@ -export-dynamic

endif