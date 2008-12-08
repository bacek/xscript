pkglib_LTLIBRARIES = xscript-mist.la

xscript_mist_la_SOURCES = xml_node.cpp state_node.cpp state_param_node.cpp \
	state_prefix_node.cpp mist_block.cpp

xscript_mist_la_LIBADD = ../library/libxscript.la
xscript_mist_la_LDFLAGS = -module -avoid-version -export-dynamic

noinst_HEADERS = mist_block.h state_node.h state_param_node.h \
	state_prefix_node.h xml_node.h

AM_CXXFLAGS = -pthread
AM_CPPFLAGS = -I../include -I../config @yandex_platform_CFLAGS@
AM_LDFLAGS = -lboost_thread -lboost_filesystem @yandex_platform_LIBS@
