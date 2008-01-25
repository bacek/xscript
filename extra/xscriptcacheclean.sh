#!/bin/sh

SLEEP=7
DIR=$1
shift

for i in `seq 0 255` ; do find $DIR/`printf '%02x\n' $i`/* -type f -amin +60 2>/dev/null | xargs -r rm -f $* ; sleep $SLEEP; done

