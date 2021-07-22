#!/bin/bash

# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

###############################################################################
#
# This file will only be read by Jenkinsfile.
#
# Deploy all artifacts
#
# Parameter marker (replaced by Jenkins):
# - %deploy_to% : i.e "nightly" "stable"
#
# Directory structure:
# $pwd/result : directory for result files
#

set +x -euo pipefail

if [ -z "$JENKINS_SERVER_COOKIE" ] ; then
    echo "error: this script has to be run by Jenkins only." >&2
    exit 1
fi

###############################################################################

deploy_to="%deploy_to%"

result_dir=$(pwd)/result
archive_dir=/srv/backup/www/s25client/$deploy_to/$(date +%Y)
updater_dir=/www/siedler25.org/nightly/s25client/$deploy_to
remote_url=https://www.siedler25.org/uploads/$deploy_to
cron_dir=/www/siedler25.org/www/docs/cron
upload_dir=/www/siedler25.org/www/uploads/$deploy_to

pushd $result_dir

mkdir -p $archive_dir
cp -av *.tar.bz2 *.zip $archive_dir/

artifacts=$(find . -maxdepth 1 -name '*.zip' -o -name '*.tar.bz2')
if [ -z "$artifacts" ] ; then
    echo "error: no artifacts were found." >&2
    exit 1
fi

mkdir -p $updater_dir
for artifact in $artifacts ; do
    echo "Processing file $artifact"

    _file=$(echo $(basename $artifact) | cut -f2- -d '_')
	VERSION=$(echo $_file | cut -f 1 -d '-')
	PLATFORM=$(echo $_file | cut -f 3 -d '-' | cut -f 1-2 -d '.')
	
    arch_dir=$PLATFORM

    echo "- Version:  $VERSION"
    echo "- Platform: $arch_dir"
    echo ""

    set -x

    _changed=1
    if [ -f $updater_dir/$arch_dir/revision ] && [ -f $arch_dir/revision ] ; then
        diff -qrN $updater_dir/$arch_dir/revision $arch_dir/revision && _changed=0 || _changed=1
    fi

    set +x

    if [ $_changed -eq 0 ] ; then
        echo "- Skipping rotation. Nothing has been changed."
    else
        echo "- Rotating tree."

        rm -rf $updater_dir/$arch_dir.5
        [ -d $updater_dir/$arch_dir.4 ] && mv -v $updater_dir/$arch_dir.4 $updater_dir/$arch_dir.5 || true
        [ -d $updater_dir/$arch_dir.3 ] && mv -v $updater_dir/$arch_dir.3 $updater_dir/$arch_dir.4 || true
        [ -d $updater_dir/$arch_dir.2 ] && mv -v $updater_dir/$arch_dir.2 $updater_dir/$arch_dir.3 || true
        [ -d $updater_dir/$arch_dir.1 ] && mv -v $updater_dir/$arch_dir.1 $updater_dir/$arch_dir.2 || true
        [ -d $updater_dir/$arch_dir ]   && mv -v $updater_dir/$arch_dir   $updater_dir/$arch_dir.1 || true
        cp -a $arch_dir $updater_dir/$arch_dir.tmp
        mv -v $updater_dir/$arch_dir.tmp $updater_dir/$arch_dir

        echo "$(date +%s);$remote_url/$(basename $artifact)" >> rapidshare-build.txt
    fi
done

if [ -f rapidshare-build.txt ] ; then
    cat rapidshare-build.txt >> $updater_dir/rapidshare.txt
    cp $updater_dir/rapidshare.txt rapidshare.txt

    echo "Uploading files."

    rsync -av *.tar.bz2 *.zip *.txt tyra4.ra-doersch.de:$upload_dir/
    ssh tyra4.ra-doersch.de "php -q ${cron_dir}/${deploy_to}sql.php"
fi

rsync -av changelog.txt tyra4.ra-doersch.de:$upload_dir/
ssh tyra4.ra-doersch.de "php -q ${cron_dir}/changelogsql.php"
