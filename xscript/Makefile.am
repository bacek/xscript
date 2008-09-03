bin_PROGRAMS = xscript-proc

xscript_proc_SOURCES = standalone_request.cpp offline_server.cpp main.cpp xslt_profiler.cpp

xscript_proc_LDADD = ../library/libxscript.la
xscript_proc_LDFLAGS = -export-dynamic

noinst_HEADERS = offline_server.h standalone_request.h xslt_profiler.h

xsldir = $(datadir)/xscript-proc
xsl_DATA = profile.xsl

dist_sysconf_DATA = offline.conf

AM_CPPFLAGS = -I@top_srcdir@/include  -I@builddir@/config -D_REENTRANT @yandex_platform_CFLAGS@
AM_CXXFLAGS = -Wall -fexceptions -frtti -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = -lboost_thread -lboost_regex -lboost_filesystem @yandex_platform_LIBS@
