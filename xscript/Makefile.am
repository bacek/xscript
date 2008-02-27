bin_PROGRAMS = xscript-proc

xscript_proc_SOURCES = request.cpp server.cpp main.cpp

xscript_proc_LDADD = ../library/libxscript.la
xscript_proc_LDFLAGS = -ldl -export-dynamic

noinst_HEADERS = request.h server.h

AM_CPPFLAGS = -I../include -I../config -D_REENTRANT
AM_CXXFLAGS = -Wall -fexceptions -frtti -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = -lboost_thread -lboost_regex -lboost_filesystem
