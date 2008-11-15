#!/bin/bash

if (( $# <= 0 )); then
	echo "usage: `basename $0` sample1.mpg [sample2.mpg [sample3.mpg ...]]"
	exit 2
fi

TESTER=""
for TRY in ./mpegvextract ./tools/mpegvextract ../tools/mpegvextract; do
	if [ -x "$TRY" ]; then
		TESTER="$TRY"
		break;
	fi
done

if [ -z "$TESTER" ]; then
	echo "ERROR: can't find mpegvextract tool, please make sure that MPEGlib"
	echo "was ./configure-d usign --enable-tools and retry"
	exit 1
fi

TCEXTRACT=$( which tcextract )
if [ ! -x $TCEXTRACT ]; then
	echo "ERROR: can't find tcextract program, so cannot go with test."
	echo "make sure that transcode is installed and tcextract is on your PATH"
	exit 4
fi

IFS=""
for sample in $*; do
	REF=$( $TCEXTRACT -x mpeg2 -i $sample 2> /dev/null | md5sum - | cut -d\- -f1 )
	NEW=$( $TESTER $sample 2> /dev/null | md5sum - | cut -d\- -f1 )
	if [ "$REF" != "$NEW" ]; then
		echo "WARNING: found a regression in \"$sample\"."
		echo "Please send a bug report at 'fromani -at- gmail -dot- com. Thanks."
	else
		echo "no problems with \"$sample\""
	fi
done
