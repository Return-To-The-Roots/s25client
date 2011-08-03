#!/bin/sh

export LD_LIBRARY_PATH=libsiedler2/src
export EF_ALLOW_MALLOC_0=1
export EF_PROTECT_FREE=0
export EF_FREE_WIPES=1

BIN=src/s25client
CMD=
ARGS=

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
			;;
		valgrind)
			CMD=valgrind
			ARGS="$ARGS --leak-check=full --log-file=s25client"
			;;
		callgrind)
			CMD=valgrind
			ARGS="$ARGS --tool=callgrind"
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

$CMD $ARGS $BIN

exit $?

