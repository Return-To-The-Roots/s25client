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
	echo "WARN: $RELEASEDEF does not contain $TARGET, using $(pwd)"
	TARGET=$(pwd)
fi

# create changelog
svn log . > $TARGET/changelog.txt

# update data
(cd $TARGET && php -q $TARGET/nightlysql.php && php -q $TARGET/changelogsql.php)

# upload data
scp $TARGET/rapidshare.txt root@tyra.ra-doersch.de:/www/siedler25.org/www/uploads
scp $TARGET/changelog.txt root@tyra.ra-doersch.de:/www/siedler25.org/www/uploads

exit 0
