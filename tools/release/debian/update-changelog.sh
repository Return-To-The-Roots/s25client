#!/bin/bash

# Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

cd $(dirname $0)

BUILD_NUMBER=$1

if [ "$USER" = "jenkins" ] ; then
	if [ -f ~/s25rttr_changelog ] ; then
		cp ~/s25rttr_changelog changelog
	fi
fi

FROM=$(head changelog -n 1 | cut -d '-' -f 2 | cut -d '.' -f 1 | cut -d ')' -f 1)
if [ -z "$FROM" ] ; then
	FROM=$(git log --format=%h | tail -n 1)
fi

HEAD=$(git log --format=%h | head -n 1)

msg=/tmp/msg.$$

# add automatic build version
if [ ! -z "$BUILD_NUMBER" ] ; then
	echo "Adding entry for automatic jenkins build $BUILD_NUMBER"
	D=$(LANG=C date +"%a, %d %b %Y %H:%M:%S %z")
	UD=$(LANG=C date +"%Y%m%d")
	echo "s25rttr ($UD-$HEAD.$BUILD_NUMBER) precise; urgency=low" > $msg
	echo "" >> $msg
	echo "  * Automatic Jenkins Build $BUILD_NUMBER" >> $msg
	echo "" >> $msg
	echo " -- Return To The Roots Team <sf-team@siedler25.org>  $D" >> $msg
	echo "" >> $msg
fi


echo "$FROM -> $HEAD"
./update-changelog.php s25rttr $FROM $HEAD >> $msg

cat changelog >> $msg
rm changelog
mv $msg changelog

./fix-changelog.php

if [ "$USER" = "jenkins" ] ; then
	cp changelog ~/s25rttr_changelog
fi

