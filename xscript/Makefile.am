lib_LTLIBRARIES = libxscript-offline.la
libxscript_offline_la_SOURCES = offline_request.cpp offline_server.cpp server_wrapper.cpp
libxscript_offline_la_LIBADD = ../library/libxscript.la

offlinedir = $(includedir)/xscript-utility
offline_DATA = server_wrapper.h

noinst_HEADERS = offline_server.h offline_request.h proc_server.h xslt_profiler.h

bin_PROGRAMS = xscript-proc

xscript_proc_SOURCES = proc_server.cpp main.cpp xslt_profiler.cpp

xscript_proc_LDADD = libxscript-offline.la
xscript_proc_LDFLAGS = -export-dynamic

xsldir = $(datadir)/xscript-proc
dist_xsl_DATA = profile.xsl

dist_sysconf_DATA = offline.conf

AM_CPPFLAGS = -I@top_srcdir@/include  -I@builddir@/config -D_REENTRANT @yandex_platform_CFLAGS@
AM_CXXFLAGS = -W -Wall -fexceptions -frtti -fPIC -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = -lboost_thread -lboost_filesystem @yandex_platform_LIBS@
