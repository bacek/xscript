pkglib_LTLIBRARIES = xscript-mist.la

xscript_mist_la_SOURCES = xml_node.cpp state_node.cpp state_param_node.cpp \
	state_prefix_node.cpp mist_block.cpp


xscript_mist_la_LIBADD = ../library/libxscript.la
xscript_mist_la_LDFLAGS = -module -avoid-version -export-dynamic

noinst_HEADERS = mist_block.h state_node.h state_param_node.h \
	state_prefix_node.h xml_node.h

AM_CPPFLAGS = -I@top_srcdir@/include -I@builddir@/config -D_REENTRANT @yandex_platform_CFLAGS@
AM_CXXFLAGS = -W -Wall -fexceptions -frtti -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = -lboost_thread -lboost_regex -lboost_filesystem @yandex_platform_LIBS@
