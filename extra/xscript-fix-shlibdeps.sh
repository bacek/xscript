# !/bin/sh

CORE_PACKAGE="libxscript"
PACKAGE=$1
FILE="debian/$PACKAGE.substvars"

if [ -f $PACKAGE ]; then
    echo "Package name must be feeded as command line parameter"
    exit 1
fi

if ! test -e $FILE; then
    echo "File $FILE not found"
    exit 1
fi

while read LINE
do
    MATCH=`echo $LINE | grep -c 'shlibs:Depends='`
    if [ "1" = $MATCH ]; then
        VERSION=`echo $LINE | sed -e 's/.*'$CORE_PACKAGE' (>=//' | sed -e 's/).*//'`
        MAJOR_VERSION=`echo $VERSION | sed -e 's/[.].*//'`
        MINOR_VERSION=`echo $VERSION | sed 's/.*\.\(.*\)\..*/\1/'`
        CORRECT_LINE=`echo -n $LINE, $CORE_PACKAGE '(<<' $MAJOR_VERSION.$(($MINOR_VERSION+1))')'`
        echo $CORRECT_LINE
    else
        echo $LINE
    fi
done < $FILE > $FILE.tmp

mv $FILE.tmp $FILE

