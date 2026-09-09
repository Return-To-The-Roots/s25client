#!/bin/bash
#
# Copyright 2023 Sam Darwin
# Copyright 2023-2024 Alexander Grund
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
#      http://www.boost.org/LICENSE_1_0.txt)
#
# Add APT keys, i.e. wrapper around add-apt-repository
# - Each argument should be a repository name
# - $NET_RETRY_COUNT is the amount of retries attempted

set -eu

function do_add_repository {
    name=$1
    echo -e "\tAdding repository $name"
    for i in $(seq "${NET_RETRY_COUNT:-3}"); do
        if [[ $i -ne 1 ]]; then
            sleep 10
            echo -e "\tRetrying"
        fi
        if sudo -E apt-add-repository -y "$name"; then
            return 0
        fi
    done
    echo "Failed adding $name"
    return 1 # Failed
}

for repo_name in "$@"; do
    [[ -n $repo_name ]] || continue
    do_add_repository "$repo_name"
done
