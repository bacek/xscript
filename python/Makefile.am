pyexec_LTLIBRARIES = xscript.la

xscript_la_SOURCES = module.cpp

xscript_la_LIBADD = ../xscript/libxscript-offline.la -lboost_python @PYTHON_LDFLAGS@
xscript_la_LDFLAGS = -module @VERSION_INFO@

AM_CPPFLAGS = -I@top_srcdir@/include -I@builddir@/config -D_REENTRANT @yandex_platform_CFLAGS@ @PYTHON_CPPFLAGS@
AM_CXXFLAGS = -W -Wall -fexceptions -frtti -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = @yandex_platform_LIBS@
