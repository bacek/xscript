EXTRA_DIST = config autogen.sh
AUTOMAKE_OPTIONS = 1.9 foreign
SUBDIRS = include library standard xscript daemon

if HAVE_LUA_BLOCK
SUBDIRS += lua-block
endif

if HAVE_MIST_BLOCK
SUBDIRS += mist-block
endif

if HAVE_HTTP_BLOCK
SUBDIRS += http-block
endif

if HAVE_TESTS
SUBDIRS += tests
endif