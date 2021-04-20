#!/bin/bash

# Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

DIR=$(cd ${0%/*} && pwd -P)

chmod 0755 $DIR/../bin/rttr.sh $DIR/../libexec/s25rttr/s25update $DIR/../bin/s25client $DIR/../bin/s25edit >/dev/null 2>/dev/null

if [ "$LD_LIBRARY_PATH" = "" ] ; then
	export LD_LIBRARY_PATH="$DIR/../lib"
else
	export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$DIR/../lib"
fi

cmd=
noupdate=0
updateonly=0
for I in $*; do
	case $I in
		debug)
			noupdate=1
			cmd=gdb
		;;
		noupdate)
			noupdate=1
		;;
		updateonly)
			updateonly=1
		;;
		gdb|ldd|valgrind|strace)
			noupdate=1
			cmd=$I
		;;
	esac
done

if [ $noupdate -eq 0 ] ; then
	if [ -f $DIR/../libexec/s25rttr/s25update ] ; then
		echo "checking for an update ..."
		cp $DIR/../libexec/s25rttr/s25update /tmp/s25update.$$
		chmod 0755 /tmp/s25update.$$
		/tmp/s25update.$$ --verbose --dir "$DIR/../"
		if [ -z "$(diff -q /tmp/s25update.$$ $DIR/../libexec/s25rttr/s25update)" ] ; then
			PARAM=noupdate
		fi
		$DIR/../bin/rttr.sh $PARAM $*
		exit $?
	fi
else
	shift
fi

if [ $updateonly -eq 0 ] ; then
	cd $DIR/../
	if ! $cmd $DIR/../bin/s25client $* ; then
		echo "An error occured: press enter to continue"
		read N
		exit 1
	fi
fi

exit 0
