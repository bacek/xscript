if HAVE_PCRE
SUBDIRS = pcre
endif

pkglib_LTLIBRARIES = \
	xscript-thrpool.la \
	xscript-xmlcache.la \
	xscript-memcache.la \
	xscript-diskcache.la \
	xscript-statistics.la \
	xscript-development.la \
	xscript-validators.la

if HAVE_XSLT_STANDARD
pkglib_LTLIBRARIES += xscript-xslt.la
endif

xscript_thrpool_la_SOURCES = thread_pool.cpp

xscript_thrpool_la_LIBADD = ../library/libxscript.la
xscript_thrpool_la_LDFLAGS = -module

xscript_memcache_la_SOURCES = doc_cache_memory.cpp doc_pool.cpp tag_key_memory.cpp module_cache.cpp

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
		cache_usage_counter_factory.cpp \
		tagged_cache_usage_counter_factory.cpp \
		response_time_counter_factory.cpp

xscript_statistics_la_LIBADD = ../library/libxscript.la
xscript_statistics_la_LDFLAGS = -module

xscript_development_la_SOURCES = development_mode.cpp

xscript_development_la_LIBADD = ../library/libxscript.la
xscript_development_la_LDFLAGS = -module


xscript_validators_la_SOURCES = range_validator.cpp

if HAVE_PCRE
xscript_validators_la_SOURCES += regex_validator.cpp
endif

xscript_validators_la_LIBADD = ../library/libxscript.la
xscript_validators_la_LDFLAGS = -module

if HAVE_XSLT_STANDARD
xscript_xslt_la_LIBADD = ../library/libxscript.la
xscript_xslt_la_LDFLAGS = -module
xscript_xslt_la_SOURCES = ""
if HAVE_PCRE
xscript_xslt_la_SOURCES += xslt_regex_extension.cpp
xscript_xslt_la_LIBADD += pcre/libpcre.la
endif
if HAVE_GLIB
xscript_xslt_la_SOURCES += xslt_extension.cpp
endif
endif

if HAVE_MEMCACHE
pkglib_LTLIBRARIES += xscript-memcached.la
xscript_memcached_la_SOURCES = doc_cache_memcached.cpp tag_key_memory.cpp
xscript_memcached_la_CFLAGS = -g -O0
xscript_memcached_la_LIBADD = ../library/libxscript.la
xscript_memcached_la_LDFLAGS = -module -lmemcached
endif

AM_CPPFLAGS = -I@top_srcdir@/include -I@builddir@/config -D_REENTRANT @yandex_platform_CFLAGS@
AM_CXXFLAGS = -W -Wall -fexceptions -frtti -fPIC -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = @BOOST_THREAD_LDFLAGS@ @BOOST_FILESYSTEM_LDFLAGS@ @yandex_platform_LIBS@ @VERSION_INFO@

EXTRA_DIST = test.conf badregexp.xml badregexp.xsl http-local-tagged.xml include.xml regexp.xml regexp.xsl \
	x-tolower.xml x-tolower.xsl x-toupper.xml x-toupper.xsl x-tests.xsl

if HAVE_TESTS

TESTS = test
check_PROGRAMS = test

noinst_LTLIBRARIES = libtest.la
libtest_la_SOURCES = doc_pool.cpp range_validator.cpp
if HAVE_PCRE
libtest_la_SOURCES += regex_validator.cpp
endif
libtest_la_LDFLAGS = -static

noinst_HEADERS = doc_pool.h range_validator.h

test_SOURCES = test_main.cpp doc_pool_test.cpp range_validator_test.cpp
if HAVE_PCRE
test_SOURCES += regex_validator_test.cpp regex_xslt_extension_test.cpp
endif
if HAVE_GLIB
test_SOURCES += xslt_extension_test.cpp
endif

test_LDADD = ../library/libxscript.la libtest.la
if HAVE_XSLT_STANDARD
if HAVE_PCRE
test_LDADD += pcre/libpcre.la
endif
test_LDADD += xscript-xslt.la
endif
test_LDFLAGS = @CPPUNIT_LIBS@ -export-dynamic

if HAVE_MEMCACHE
test_SOURCES += memcached_test.cpp
endif

endif

CLEANFILES = default.log
