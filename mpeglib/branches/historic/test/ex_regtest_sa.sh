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
	echo "was ./configure-d using --enable-tools and retry"
	exit 1
fi

EXTRACT=""
for TRY in ./extract_mpeg2 ./test/extract_mpeg2 ../test/extract_mpeg2; do
	if [ -x "$TRY" ]; then
		EXTRACT="$TRY"
		break;
	fi
done

if [ ! -x $EXTRACT ]; then
	echo "ERROR: can't find extract_mpeg2 program, so cannot go with test."
	echo "make sure that MPEGlib was ./configure-d using --enable-tools and retry"
	exit 4
fi

IFS=""
for sample in $*; do
	REF=$( $EXTRACT < "$sample" 2> /dev/null | md5sum - | cut -d\- -f1 )
	NEW=$( $TESTER "$sample" 2> /dev/null | md5sum - | cut -d\- -f1 )
	if [ "$REF" != "$NEW" ]; then
		echo "WARNING: found a regression in \"$sample\"."
                echo "Please send a bug report at 'fromani -at- gmail -dot- com. Thanks."
	else
		echo "no problems with \"$sample\""
	fi
done
