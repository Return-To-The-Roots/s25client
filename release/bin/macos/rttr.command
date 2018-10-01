#!/bin/sh

# Get absolute path to scripts folder
scriptFolder="$(cd "$(dirname "$0")" && pwd)"
# Get absolute path to script
scriptPath="$scriptFolder/$(basename "$0")"
cd "$scriptFolder" || exit 1

if [ ! "$1" = "interminal" ] ; then
	echo "#!/bin/sh" > /tmp/rttr.command
	echo "\"$scriptPath\" interminal" >> /tmp/rttr.command
	chmod 0755 /tmp/rttr.command
	open rttr.terminal
	sleep 5
	rm -f /tmp/rttr.command
	exit $?
fi

export DYLD_LIBRARY_PATH="$scriptFolder:$DYLD_LIBRARY_PATH"

chmod 0755 ./rttr.command ./libexec/s25rttr/s25update ./bin/s25client ./bin/s25edit ./libexec/s25rttr/sound-convert

RTTR_TEST_FILEA="share/s25rttr/S2/DATA/CREDITS.LST"
RTTR_TEST_FILEB="share/s25rttr/S2/GFX/PALETTE/PAL5.BBM"

if [ ! -f "$RTTR_TEST_FILEA" ] || [ ! -f "$RTTR_TEST_FILEB" ]; then
	
	while [ ! -d /Volumes/S2_GOLD ] && ([ ! -f "$RTTR_TEST_FILEA" ] || [ ! -f "$RTTR_TEST_FILEB" ]); do
		echo "Couldn't find data files for Settlers II"
		echo "Please copy the folders \"DATA\" and \"GFX\" from your Settlers II install to"
		echo "\"$scriptFolder/share/s25rttr/S2\" or"
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

if [ ! "$2" = "noupdate" ] ; then
	if [ -f ./libexec/s25rttr/s25update ] ; then
		(cd ../../../ && ./s25client.app/Contents/MacOS/libexec/s25rttr/s25update -d "$PWD")
		chmod 0755 ./rttr.command
		./rttr.command interminal noupdate
		exit $?
	fi
fi

./bin/s25client

exit $?
