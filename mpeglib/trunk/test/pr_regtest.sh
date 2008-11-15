#!/bin/bash

if (( $# <= 0 )); then
	echo "usage: `basename $0` sample1.mpg"
	exit 2
fi

TESTER=""
for TRY in ./mpprobe ./tools/mpprobe ../tools/mpprobe; do
	if [ -x "$TRY" ]; then
		TESTER="$TRY"
		break;
	fi
done

if [ -z "$TESTER" ]; then
	echo "ERROR: can't find mpprobe tool, please make sure that MPEGlib"
	echo "was ./configure-d usign --enable-tools and retry"
	exit 1
fi

TCPROBE=$( which tcprobe )
if [ ! -x $TCPROBE ]; then
	echo "ERROR: can't find tcextract program, so cannot go with test."
	echo "make sure that transcode is installed and tcextract is on your PATH"
	exit 4
fi

# FIXME: yeah, this is an ugly hack. Rewrite ASAP
IFS=""
for sample in $*; do
	TARGET=/tmp/probe-$( basename $sample )
	$TESTER  -i $sample 2> /dev/null > $TARGET-new
	$TCPROBE -i $sample 2> /dev/null > $TARGET-old
	if diff $TARGET-old $TARGET-new > $TARGET-diff; then
		echo "no problems with $sample"
	else
		echo "found a diversion in $sample"
		cat $TARGET-diff
	fi
	rm -f $TARGET-*
done
