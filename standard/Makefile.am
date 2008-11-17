pkglib_LTLIBRARIES = \
	xscript-thrpool.la \
	xscript-xmlcache.la \
	xscript-memcache.la \
	xscript-diskcache.la \
	xscript-statistics.la \
	xscript-development.la \
	xscript-validators.la

xscript_thrpool_la_SOURCES = thread_pool.cpp

xscript_thrpool_la_LIBADD = ../library/libxscript.la
xscript_thrpool_la_LDFLAGS = -module

xscript_memcache_la_SOURCES = doc_cache_memory.cpp doc_pool.cpp

xscript_memcache_la_LIBADD = ../library/libxscript.la
xscript_memcache_la_LDFLAGS = -module

xscript_diskcache_la_SOURCES = doc_cache_disk.cpp

xscript_diskcache_la_LIBADD = ../library/libxscript.la
xscript_diskcache_la_LDFLAGS = -module

xscript_xmlcache_la_SOURCES = xml_cache.cpp

xscript_xmlcache_la_LIBADD = ../library/libxscript.la
xscript_xmlcache_la_LDFLAGS = -module

xscript_statistics_la_SOURCES = \
		average_counter_factory.cpp \
		simple_counter_factory.cpp \
		cache_counter_factory.cpp \
		cache_usage_counter_factory.cpp
xscript_statistics_la_LDFLAGS = -module

xscript_development_la_SOURCES = development_mode.cpp

xscript_development_la_LIBADD = ../library/libxscript.la
xscript_development_la_LDFLAGS = -module


xscript_validators_la_SOURCES = \
	range_validator.cpp \
	regex_validator.cpp

xscript_validators_la_LIBADD = ../library/libxscript.la
xscript_validators_la_LDFLAGS = -module

AM_CPPFLAGS = -I@top_srcdir@/include -I@builddir@/config -D_REENTRANT @yandex_platform_CFLAGS@
AM_CXXFLAGS = -W -Wall -fexceptions -frtti -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = -lboost_thread -lboost_regex -lboost_filesystem @yandex_platform_LIBS@

EXTRA_DIST = test.conf

if HAVE_TESTS

TESTS = test
check_PROGRAMS = test

noinst_LTLIBRARIES = libtest.la
libtest_la_SOURCES = doc_pool.cpp range_validator.cpp regex_validator.cpp
libtest_la_LDFLAGS = -static

noinst_HEADERS = doc_pool.h range_validator.h

test_SOURCES = test_main.cpp \
			   doc_pool_test.cpp \
			   range_validator_test.cpp \
			   regex_validator_test.cpp

test_LDADD = ../library/libxscript.la libtest.la
test_LDFLAGS = @CPPUNIT_LIBS@ -export-dynamic

endif
