#!/bin/sh
#Starting script for xscript

#Main options
ETCDIR='/etc'
THIS=`basename $0 | sed 's/^[SK][0123456789]*b/b/'`
XSCRIPT_CONFIG=$1
LOCKFILE=$2/xscriptstart.pid
LOGFILE=$3/xscriptstart.log
RDELAY=1

#Ulimit option
COREDIR=/var/tmp/core-files
ulimit -s 1024
ulimit -c unlimited


if [ -e $XSCRIPT_CONFIG  ]; then
    touch $LOCKFILE
    trap "rm $LOCFILE 2>/dev/null" 0
    ps ax | grep "xscriptstart.sh" | grep -v grep | awk '{print $1}' > $LOCKFILE
    if [ -d "$COREDIR" ]; then
	mkdir -p $COREDIR 2>/dev/null
    fi
    while [ -f $LOCKFILE ]; do
	echo "/usr/bin/xscript-bin --config=$XSCRIPT_CONFIG"
        /usr/bin/xscript-bin --config=$XSCRIPT_CONFIG >> $LOGFILE 2>&1
        echo "on `date` ::$THIS:: $THIS on `hostname` restarted"  >>$LOGFILE
        sleep $RDELAY
    done
else
    echo "$XSCRIPT_CONFIG don't found"
    exit 1
fi
