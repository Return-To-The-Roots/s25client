#!/bin/bash

TYPE=$1

usage()
{
	echo "$0 [nightly|stable]"
	exit 1
}

if [ -z "$TYPE" ] ; then
	usage
fi

cd $(dirname $0)

RELEASEDEF=release.$TYPE.def
source $RELEASEDEF || exit 1

if [ ! -d "$TARGET" ] ; then
	echo "WARN: $RELEASEDEF does not contain TARGET, using $(pwd)"
	TARGET=$(pwd)
fi

# create changelog
echo "Creating Changelog ..."
git log --submodule --no-max-parents | sed -r "s#commit (.*)#commit \1\nRepository: $(git remote -v | head -n 1 | egrep -o "([^/]*)\.git" | sed s/.git//g)#g" > ${UPLOADFILE2}changelog.txt || exit 2
git submodule foreach 'git log --submodule --no-max-parents | sed -r "s#commit (.*)#commit \1\nRepository: $(git remote -v | head -n 1 | egrep -o "([^/]*)\.git" | sed s/.git//g)#g"' >> ${UPLOADFILE2}changelog.txt || exit 2


cp ${UPLOADFILE2}changelog.txt changelog-$TYPE.txt
cp ${UPLOADFILE2}rapidshare.txt rapidshare-$TYPE.txt

# upload data
echo "Uploading Data ..."
scp -4 ${UPLOADFILE2}rapidshare.txt $UPLOADTARGET$UPLOADTO || exit 3
scp -4 ${UPLOADFILE2}changelog.txt $UPLOADTARGET$UPLOADTO || exit 4

if [ ! -z "$UPLOADSCRIPT" ] ; then
	echo "Running post upload script ..."
	bash -c "$UPLOADSCRIPT"
fi

exit 0
