#!/bin/sh

# Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

export LD_LIBRARY_PATH=libsiedler2/src
export EF_ALLOW_MALLOC_0=1
export EF_PROTECT_FREE=0
export EF_FREE_WIPES=1

BIN=@RTTR_BINDIR@/s25client
CMD=
ARGS=
BINARGS=
BUGLE=

while test $# != 0 ; do
	case $1 in
		--*=*)
			ac_option=`expr "X$1" : 'X\([^=]*\)='`
			ac_optarg=`expr "X$1" : 'X[^=]*=\(.*\)'`
			ac_shift=2
			;;
		*)
			ac_option=$1
			ac_optarg=$2
			ac_shift=1
			;;
	esac

	case $ac_option in
		-l | --load)
			BINARGS="$ac_optarg"
			if [ -n "$ac_optarg" ]
			then
				ac_shift=2
			fi
			;;
		-n | --nowait)
			NOWAIT=1
			;;
		-m | --makeargs)
			MAKEARGS="$ac_optarg"
			if [ -n "$ac_optarg" ]
			then
				ac_shift=2
			fi
			;;
		debug)
			CMD=gdb
			BIN="--args $BIN"
			;;
		valgrind)
			CMD=valgrind
			ARGS="$ARGS --leak-check=full --log-file=s25client"
			;;
		callgrind)
			CMD=valgrind
			ARGS="$ARGS --tool=callgrind --instr-atstart=no"
			;;
		--bugle-chain)
			BUGLE="$ac_optarg"
			ac_shift=2
			;;
		bugle)
			BUGLE=showcalltimes
			;;
		ddd)
			CMD=ddd
			;;
		mpatrol)
			CMD=mpatrol
			ARGS="$ARGS -C"
			;;
		*)
			echo "Unknown option: $ac_option"
			exit 1
			;;
	esac

	shift $ac_shift
done

if [ -f /proc/cpuinfo ] ; then
	CPU_COUNT=`grep -ce '^processor' /proc/cpuinfo`

	if test "$CPU_COUNT" -gt 1; then
		MAKEARGS="-j $((1+$CPU_COUNT)) $MAKEARGS"
	fi
elif [ $(sysctl -n hw.ncpu) -gt 1 ]; then
	MAKEARGS="-j $((1 + $(sysctl -n hw.ncpu))) $MAKEARGS"
fi

make $MAKEARGS

if [ $? != 0 ] ; then
	exit 1
fi

case $CMD in
	gdb)
		if [ -n "$NOWAIT" ] ; then
			ARGS="$ARGS -x start.gdb"
		fi
		;;
esac

if [ -n "$BUGLE" ]; then
	BUGLE_FILTERS="bugle/filters" BUGLE_STATISTICS="bugle/statistics" BUGLE_CHAIN="$BUGLE" LD_PRELOAD=libbugle.so $CMD $ARGS $BIN $BINARGS
else
	$CMD $ARGS $BIN $BINARGS
fi

exit $?

