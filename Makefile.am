AUTOMAKE_OPTIONS = 1.9 foreign
ACLOCAL_AMFLAGS = -I config
SUBDIRS = include library standard xscript daemon

EXTRA_DIST = ChangeLog Doxyfile AUTHORS COPYING INSTALL NEWS README \
	autogen.sh config/cppunit.m4 config/ax_check_cppunit.m4 \
	config/settings.h.in config/settings.h debian/rules debian/changelog \
	debian/compat debian/xscript-daemon.files debian/control \
	debian/xscript-daemon.init debian/xscript-utility.files \
	debian/xscript-standard.preinst debian/libxscript-dev.files \
	debian/xscript-daemon.preinst debian/xscript-standard.files \
	debian/libxscript.files examples/control-logrotate.xml \
	examples/control-status-info.xml examples/control-tagged-cache-stat.xml \
	examples/control-xml-cache-stat.xml examples/data/1.xml \
	examples/data/2.xml examples/file-include.xml examples/file-load.xml \
	examples/file-tag.xml examples/http_1.xml examples/http_2.xml \
	examples/http_3.xml examples/http_4.xml examples/http-local-tagged.xml \
	examples/include.xml examples/simple.xml examples/xinclude.xml \
	extra/xscript-cache-clean extra/xscriptcacheclean.sh \
	extra/xscriptstart.sh extra/xscript-fix-shlibdeps.sh

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
