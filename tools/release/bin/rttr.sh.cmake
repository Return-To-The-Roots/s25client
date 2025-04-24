#!/bin/bash

# Copyright (C) 2005 - 2025 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

DIR=$(cd "${0%/*}" && pwd -P)
ROOT_DIR=$(dirname "$DIR")

chmod 0755 "$ROOT_DIR/bin/rttr.sh" "$ROOT_DIR/libexec/s25rttr/s25update" "$ROOT_DIR/bin/s25client" "$ROOT_DIR/bin/s25edit" >/dev/null 2>/dev/null

if [ "$LD_LIBRARY_PATH" = "" ] ; then
	export LD_LIBRARY_PATH="$ROOT_DIR/lib"
else
	export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$ROOT_DIR/lib"
fi

cmd=
noupdate=0
updateonly=0
for I in "$@"; do
	case "$I" in
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
	if [ -f "$ROOT_DIR/libexec/s25rttr/s25update" ] ; then
		echo "checking for an update ..."
		cp "$ROOT_DIR/libexec/s25rttr/s25update" /tmp/s25update.$$
		chmod 0755 /tmp/s25update.$$
		/tmp/s25update.$$ --dir "$ROOT_DIR/" @STABLE_PARAM@
		if diff -q /tmp/s25update.$$ "$ROOT_DIR/libexec/s25rttr/s25update" &> /dev/null; then
			PARAM=noupdate
		else
			PARAM=
		fi
		"$ROOT_DIR/bin/rttr.sh" $PARAM "$@"
		exit $?
	fi
else
	shift
fi

if [ $updateonly -eq 0 ] ; then
	if ! { cd "$ROOT_DIR" && $cmd "$ROOT_DIR/bin/s25client" "$@"; } ; then
		echo "An error occured: press enter to continue"
		read N
		exit 1
	fi
fi

exit 0
