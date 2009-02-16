pyexec_LTLIBRARIES = xscript_server.la

xscript_server_la_SOURCES = module.cpp

xscript_server_la_LIBADD = ../xscript/libxscript-offline.la -lboost_python @PYTHON_LDFLAGS@
xscript_server_la_LDFLAGS = -module @VERSION_INFO@

AM_CPPFLAGS = -I@top_srcdir@/include -I@builddir@/config -D_REENTRANT @yandex_platform_CFLAGS@
AM_CXXFLAGS = -W -Wall -fexceptions -frtti -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = @yandex_platform_LIBS@
