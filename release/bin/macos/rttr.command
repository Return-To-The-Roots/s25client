#!/bin/bash

cd $(dirname $0)

if [ ! "$1" = "interminal" ] ; then
	echo "$0 interminal" > /tmp/rttr.command
	chmod 0755 /tmp/rttr.command
	open rttr.terminal
	sleep 2
	rm -f /tmp/rttr.command
	exit $?
fi 

chmod 0755 ../rttr.command ../share/s25rttr/RTTR/s25update ../bin/s25client ../share/s25rttr/RTTR/sound-convert >/dev/null 2>/dev/null

RTTR_TEST_FILEA="share/s25rttr/S2/DATA/CREDITS.LST"
RTTR_TEST_FILEB="share/s25rttr/S2/GFX/PALETTE/PAL5.BBM"

if ([ ! -f "$RTTR_TEST_FILEA" ] || [ ! -f "$RTTR_TEST_FILEB" ]); then
	
	while ([ ! -d /Volumes/S2_GOLD ] && ([ ! -f "$RTTR_TEST_FILEA" ] || [ ! -f "$RTTR_TEST_FILEB" ]) ) ; do
		echo "Die Siedler II Dateien sind noch nicht vorhanden"
		echo "Entweder kopieren sie den \"DATA\" und \"GFX\" Ordner ihrer S2-Installation nach \"$(dirname $0)/share/s25rttr/S2\" oder"
		echo "legen sie bitte die Siedler II Gold CD in ihr Laufwerk und bestätigen sie mit \"Enter\""
		read
	done
	
	if ([ ! -f "$RTTR_TEST_FILEA" ] || [ ! -f "$RTTR_TEST_FILEB" ]) ; then
		echo "Kopiere Dateien"
		
		if [ ! -f "$RTTR_TEST_FILEA" ] ; then
			cp -rv /Volumes/S2_GOLD/S2/DATA share/s25rttr/S2
			if [ ! $? -eq 0 ] ; then
				echo "Fehlgeschlagen"
				rm -rf share/s25rttr/S2/DATA
				exit 1
			fi
		fi

		if [ ! -f "$RTTR_TEST_FILEB" ] ; then
			cp -rv /Volumes/S2_GOLD/S2/GFX share/s25rttr/S2
			if [ ! $? -eq 0 ] ; then
				echo "Fehlgeschlagen"
				rm -rf share/s25rttr/S2/GFX
				exit 1
			fi
		fi

		echo "Fertig"
	fi
fi

if [ ! "$1" = "noupdate" ] ; then
	if [ -f ./share/s25rttr/RTTR/s25update ] ; then
		(cd ../../../ && ./s25client.app/Contents/MacOS/share/s25rttr/RTTR/s25update -d $PWD)
	fi
fi

chmod 0755 ./rttr.command ./share/s25rttr/RTTR/s25update ./bin/s25client ./share/s25rttr/RTTR/sound-convert >/dev/null 2>/dev/null

./bin/s25client

exit $?
