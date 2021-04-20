#!/bin/bash

# Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

###############################################################################
#
# This file will only be read by Jenkinsfile.
#
# Create nightly updater tree's for each architecture
#
# Directory structure:
# $pwd/source : directory for source files
# $pwd/result : directory for result files
#

set +x -euo pipefail

if [ -z "$JENKINS_SERVER_COOKIE" ] ; then
    echo "error: this script has to be run by Jenkins only." >&2
    exit 1
fi

###############################################################################

src_dir=$(pwd)/source
result_dir=$(pwd)/result

pushd $result_dir

RTTR_SAVEGAME_VERSION=$(grep "; // SaveGameVersion -- " $src_dir/libs/s25main/Savegame.cpp | cut -d " " -f 6 | cut -d \; -f 1)

echo "Savegame Version: $RTTR_SAVEGAME_VERSION"
echo ""

artifacts=$(find . -maxdepth 1 -name '*.zip' -o -name '*.tar.bz2')
if [ -z "$artifacts" ] ; then
    echo "error: no artifacts were found." >&2
    exit 1
fi

for artifact in $artifacts ; do
    echo "Processing file $artifact"
    _version=$(echo $(basename $artifact) | cut -f2- -d '_' | cut -f 1-2 -d '.')
    RTTR_VERSION=$(echo $_version | cut -f 1 -d '-')
    RTTR_REVISION=$(echo $_version | cut -f 2 -d '-')

    _platform=$(echo $_version | cut -f 3 -d '-')
    PLATFORM_NAME=$(echo $_platform | cut -f 1 -d '.')
    PLATFORM_ARCH=$(echo $_platform | cut -f 2 -d '.')

    ARCHIVE_TYPE=$(echo $(basename $artifact) | cut -f 3- -d '.')

    echo "- Version:       $RTTR_VERSION"
    echo "- Revision:      $RTTR_REVISION"
    echo "- Platform Name: $PLATFORM_NAME"
    echo "- Platform Arch: $PLATFORM_ARCH"
    echo "- Archive Type:  $ARCHIVE_TYPE"
    echo ""

    arch_dir=$(pwd)/$PLATFORM_NAME.$PLATFORM_ARCH

    echo "Creating updater tree"

    echo "- creating packed location"
    packed_dir=$arch_dir/packed
    mkdir -p $packed_dir

    cp -av $artifact $packed_dir/s25rttr.${ARCHIVE_TYPE}

    echo "- unpacking archive"
    unpacked_dir=$arch_dir/unpacked/s25rttr_${RTTR_VERSION}
    mkdir -p $unpacked_dir

    case $ARCHIVE_TYPE in
        zip)
            unzip -q -d $unpacked_dir $artifact
        ;;
        tar.bz2)
            tar -C $unpacked_dir -xf $artifact
        ;;
        *)
            echo "error: unknown archive type '$ARCHIVE_TYPE'." >&2
            exit 1
        ;;
    esac

    # fix directory layout
    # (apple does not have that directory inside, so this is optional)
    if [ -d $unpacked_dir/s25rttr_${RTTR_VERSION} ] ; then
        mv -v $unpacked_dir/s25rttr_${RTTR_VERSION} $unpacked_dir.tmp
        rm -rf $unpacked_dir
        mv -v $unpacked_dir.tmp $unpacked_dir
    fi

    echo "- creating tree"
    updater_dir=$arch_dir/updater
    mkdir -p $updater_dir

    # fastcopy files (only dirs and files, no symlinks)
    echo "  - copying directories and files"
    (cd $unpacked_dir && find -type d -a -exec mkdir -p $updater_dir/{} \;)
    (cd $unpacked_dir && find -type f -a -exec cp {} $updater_dir/{} \;)

    # note symlinks
    echo "  - find symlinks"
    symlink_file=/tmp/links.$$
    echo -n > $symlink_file
    (cd $unpacked_dir && find -type l -exec bash -c 'echo "{} $(readlink {})"' \;) | tee $symlink_file

    # note hashes
    echo "  - get file hashes"
    files_file=/tmp/files.$$
    (cd $updater_dir && md5deep -r -l .) > $files_file

    # bzip files
    echo "  - compress files"
    find $updater_dir -type f -exec bzip2 {} \;

    # move file lists
    echo "  - finalize tree"
    mv $symlink_file $updater_dir/links
    mv $files_file $updater_dir/files
    echo $RTTR_SAVEGAME_VERSION > $updater_dir/savegameversion

    # create human version notifier
    echo "$RTTR_VERSION"  > $arch_dir/version-${RTTR_VERSION}
    echo "$RTTR_REVISION" > $arch_dir/revision-${RTTR_REVISION}
    echo "$RTTR_REVISION" > $arch_dir/revision
    echo "$RTTR_VERSION"  > $arch_dir/version
    echo "${RTTR_VERSION}-${RTTR_REVISION}" > $arch_dir/full-version

    echo ""
done
