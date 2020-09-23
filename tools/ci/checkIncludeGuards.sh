#!/bin/bash

set -euo pipefail

header_file="${1:?header_file missing}"

while read -r line; do
    if [[ "$line" =~ ^// ]] || [[ "$line" =~ ^$ ]]; then
        continue
    elif [[ "$line" =~ ^/\* ]] || [[ "$line" =~ ^\* ]]; then
        continue
    elif [[ "$line" != "#pragma once" ]]; then
        if grep -q "#pragma once" "$header_file"; then
            echo "$header_file has misplaced #pragma once. First line: '$line'"
        else
            echo "$header_file has no #pragma once"
        fi
        exit 1
    else
        break
    fi
done < "$header_file"
