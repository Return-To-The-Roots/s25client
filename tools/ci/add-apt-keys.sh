#!/bin/bash
#
# Copyright 2023-2024 Alexander Grund
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
#      http://www.boost.org/LICENSE_1_0.txt)
#
# Add APT keys
# - Each argument should be a key URL or a key ID (0x...)
# - $NET_RETRY_COUNT is the amount of retries attempted

set -eu

if curl --retry-all-errors 2>&1 | grep -iq unknown; then
    # --retry-all-errors not available
    curl_extra_options=""
else
    curl_extra_options="--retry-all-errors"
fi

function do_add_key
{
    key_url=$1
    # If a keyserver URL (e.g. http://keyserver.ubuntu.com/pks/lookup?op=get&search=0x1E9377A2BA9EF27F)
    # use the hash as the filename,
    # if it is an ID, e.g. 0x1E9377A2BA9EF27F, use it as the filename, and http://keyserver.ubuntu.com/pks/lookup as the URL
    # else assume the URL contains a filename, e.g. https://apt.llvm.org/llvm-snapshot.gpg.key
    if [[ "$key_url" =~ .*keyserver.*search=0x([A-F0-9]+) ]]; then
        keyfilename="${BASH_REMATCH[1]}.key"
    elif [[ $key_url =~ ^0x[A-F0-9]+$ ]]; then
        keyfilename=${key_url#0x}.key
        key_server="keyserver.boost.org"
        key_url="https://$key_server/pks/lookup?op=get&search=$key_url"
    else
        keyfilename=$(basename -s .key "$key_url")
    fi
    echo -e "\tDownloading APT key from '$key_url' to '$keyfilename'"
    if ! curl ${curl_extra_options} --connect-timeout 15 -sSL --retry "${NET_RETRY_COUNT:-5}" "$key_url" | sudo gpg --dearmor -o "/etc/apt/trusted.gpg.d/${keyfilename}"; then
        echo "Failed downloading $keyfilename"
        return 1
    fi
}

for key_url in "$@"; do
    [[ -n $key_url ]] || continue
    do_add_key "$key_url"
done
