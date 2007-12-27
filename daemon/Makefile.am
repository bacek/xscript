bin_PROGRAMS = xscript-bin

xscript_bin_SOURCES = parser.cpp request.cpp server.cpp main.cpp 

xscript_bin_LDADD = ../library/libxscript.la
xscript_bin_LDFLAGS = -lfcgi -lfcgi++ -export-dynamic

noinst_HEADERS = parser.h request.h server.h functors.h

dist_sysconf_DATA = xscript.conf xscript-ulimits.conf

AM_CPPFLAGS = -I../include -I../config -D_REENTRANT
AM_CXXFLAGS = -Wall -fexceptions -frtti -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = -lboost_thread -lboost_regex -lboost_filesystem
