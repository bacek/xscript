AUTOMAKE_OPTIONS = 1.9 foreign
SUBDIRS = include library standard xscript daemon
EXTRA_DIST = config autogen.sh extra debian examples Doxyfile

if HAVE_LUA_BLOCK
SUBDIRS += lua-block
endif

if HAVE_MIST_BLOCK
SUBDIRS += mist-block
endif

if HAVE_HTTP_BLOCK
SUBDIRS += http-block
endif

if HAVE_FILE_BLOCK
SUBDIRS += file-block
endif

if HAVE_TESTS
SUBDIRS += tests
endif


coverage: 
	lcov -c -d . -o lcov.tmp
	lcov -r lcov.tmp '/usr*' -o lcov.info
	genhtml -o lcov lcov.info

