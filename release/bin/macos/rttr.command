#!/bin/sh

cd $(dirname $0)

if [ ! "$1" = "interminal" ] ; then
	echo "#!/bin/sh" > /tmp/rttr.command
	echo "$0 interminal" >> /tmp/rttr.command
	chmod 0755 /tmp/rttr.command
	open rttr.terminal
	sleep 2
	rm -f /tmp/rttr.command
	exit $?
fi

chmod 0755 ../rttr.command ../share/s25rttr/RTTR/s25update ../bin/s25client ../share/s25rttr/RTTR/sound-convert >/dev/null 2>&1

RTTR_TEST_FILEA="share/s25rttr/S2/DATA/CREDITS.LST"
RTTR_TEST_FILEB="share/s25rttr/S2/GFX/PALETTE/PAL5.BBM"

if [ ! -f "$RTTR_TEST_FILEA" ] || [ ! -f "$RTTR_TEST_FILEB" ]; then
	
	while [ ! -d /Volumes/S2_GOLD ] && ([ ! -f "$RTTR_TEST_FILEA" ] || [ ! -f "$RTTR_TEST_FILEB" ]); do
		echo "Couldn't find data files for Settlers II"
		echo "Please copy the folders \"DATA\" and \"GFX\" from your Settlers II install to"
		echo "\"$(dirname $0)/share/s25rttr/S2\" or"
		echo "insert the Settlers II Gold CD in your drive and hit \"Enter\"."
		read
	done

	if [ ! -f "$RTTR_TEST_FILEA" ] || [ ! -f "$RTTR_TEST_FILEB" ]; then
		echo "Copying files ..."

		if [ ! -f "$RTTR_TEST_FILEA" ] ; then
			if ! cp -rv /Volumes/S2_GOLD/S2/DATA share/s25rttr/S2; then
				echo "Error copying files" >&2
				rm -rf share/s25rttr/S2/DATA
				exit 1
			fi
		fi

		if [ ! -f "$RTTR_TEST_FILEB" ] ; then
			if ! cp -rv /Volumes/S2_GOLD/S2/GFX share/s25rttr/S2; then
				echo "Error copying files" >&2
				rm -rf share/s25rttr/S2/GFX
				exit 1
			fi
		fi

		echo "Done."
	fi
fi

if [ ! "$1" = "noupdate" ] ; then
	if [ -f ./share/s25rttr/RTTR/s25update ] ; then
		(cd ../../../ && ./s25client.app/Contents/MacOS/share/s25rttr/RTTR/s25update -d $PWD)
	fi
fi

chmod 0755 ./rttr.command ./share/s25rttr/RTTR/s25update ./bin/s25client ./share/s25rttr/RTTR/sound-convert >/dev/null 2>&1

./bin/s25client

exit $?
