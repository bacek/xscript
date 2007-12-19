pkglib_LTLIBRARIES = xscript-thrpool.la xscript-xmlcache.la xscript-memcache.la xscript-diskcache.la

xscript_thrpool_la_SOURCES = thread_pool.cpp

xscript_thrpool_la_LIBADD = ../library/libxscript.la
xscript_thrpool_la_LDFLAGS = -module

xscript_memcache_la_SOURCES = tagged_cache_memory.cpp

xscript_memcache_la_LIBADD = ../library/libxscript.la
xscript_memcache_la_LDFLAGS = -module

xscript_diskcache_la_SOURCES = tagged_cache_disk.cpp

xscript_diskcache_la_LIBADD = ../library/libxscript.la
xscript_diskcache_la_LDFLAGS = -module

xscript_xmlcache_la_SOURCES = xml_cache.cpp

xscript_xmlcache_la_LIBADD = ../library/libxscript.la
xscript_xmlcache_la_LDFLAGS = -module

AM_CPPFLAGS = -I../include -I../config -D_REENTRANT
AM_CXXFLAGS = -Wall -fexceptions -frtti -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = -lboost_thread -lboost_regex -lboost_filesystem
