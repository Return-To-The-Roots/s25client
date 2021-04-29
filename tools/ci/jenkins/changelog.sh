#!/bin/bash

# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

###############################################################################
#
# This file will only be read by Jenkinsfile.
#
# Create changelog.txt from source directory
#
# Directory structure:
# $pwd/source : source directory
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

pushd $src_dir

echo "Generating Changelog"
git log --submodule --no-max-parents | sed -r "s#^commit (.*)#commit \1\nRepository: $(git remote -v | head -n 1 | egrep -o "([^/]*)\.git" | sed s/.git//g)#g" > $result_dir/changelog.txt || exit 2
git submodule foreach 'git log --submodule --no-max-parents | sed -r "s#^commit (.*)#commit \1\nRepository: $(git remote -v | head -n 1 | egrep -o "([^/]*)\.git" | sed s/.git//g)#g"' >> $result_dir/changelog.txt || exit 2
echo "Done"
