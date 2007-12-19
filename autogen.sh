#! /bin/sh

rm -f config.cache
rm -f config.log

aclocal-1.9 -I config
autoheader
libtoolize --force
automake-1.9 --force --add-missing
autoconf
