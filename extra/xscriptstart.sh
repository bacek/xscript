#!/bin/sh
#Starting script for xscript

#Main options
ETCDIR='/etc'
NAME=$1
WORK_DIR=$ETCDIR/$NAME

XSCRIPT_CONFIG=$WORK_DIR/xscript.conf
CUSTOM_FILE_TEMPLATE=xscript-*\.conf

LOCKFILE="/var/run/$NAME/xscriptstart.pid"
LOGFILE="/var/log/$NAME/xscriptstart.log"
RDELAY=1
EXIT_TIMEOUT=10


for CUSTOM_CONFIG in $WORK_DIR/$CUSTOM_FILE_TEMPLATE
do
    if [ -r $CUSTOM_CONFIG ]; then
	. $CUSTOM_CONFIG
    fi
done

if [ -e $XSCRIPT_CONFIG  ]; then
    touch $LOCKFILE
    trap "rm $LOCKFILE 2>/dev/null" 0
    ps ax | grep "xscriptstart.sh $NAME$" | grep -v grep | awk '{print $1}' > $LOCKFILE
    if [ -d "$COREDIR" ]; then
	mkdir -p $COREDIR 2>/dev/null
    fi
    SLEEP_TIMEOUT=0
    while [ -f $LOCKFILE ]; do
	sleep $SLEEP_TIMEOUT
	STARTTIME=`date +%s`
	echo "/usr/bin/xscript-bin --config=$XSCRIPT_CONFIG"
        /usr/bin/xscript-bin --config=$XSCRIPT_CONFIG >> $LOGFILE 2>&1
        echo "on `date` ::xscriptstart.sh:: xscriptstart.sh on `hostname` restarted"  >>$LOGFILE
        NOWTIME=`date +%s`
	if [ $SLEEP_TIMEOUT = 0 -a $EXIT_TIMEOUT -gt 0 ]; then
	   WORKTIME=$(($NOWTIME-$STARTTIME))
	   if [ $WORKTIME -lt $EXIT_TIMEOUT ]; then
		echo "Fast xscript crash.  Exit 1"
		exit 1
	   fi
	fi
	SLEEP_TIMEOUT=$RDELAY
    done
else
    echo "Can not find $XSCRIPT_CONFIG"
    exit 1
fi
