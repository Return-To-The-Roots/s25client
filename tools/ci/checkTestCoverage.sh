#!/bin/bash

set -euo pipefail

export PATH="/tmp/lcov/bin:$PATH"
SRC_PATH="$PWD/tests"

result=0
lcov --extract coverage.info '*/tests/*' -o testcov.info &> /dev/null
while read -r line; do
    IFS='|' read -ra data <<< "$line"
    if [[ ${#data[@]} == 4 ]]; then
        file=$(sed 's/[[:space:]]*$//' <<< "${data[0]}")
        lineCov=$(echo "${data[1]}" | sed 's/^[[:space:]]*//' | cut -d ' ' -f1)
        filepath="$SRC_PATH/$file"
        if [ -f "$filepath" ] && ! [[ "$lineCov" =~ "100%" ]]; then
            echo "$file has only $lineCov coverage. Uncovered lines:"
            lcov --extract testcov.info '*'"/$file" |& grep 'DA:.*,0' | sed -E 's/DA:(.*),0/\1/g'
            result=1
        fi
    fi
done < <(lcov --list testcov.info | grep '%')

if [ $result -ne 0 ]; then
  echo "Found uncovered lines in tests. This is usually an error as all test code should be executed"
  echo "In case those lines are unreachable by design (e.g. output operators or failure handling)"
  echo "you can wrap those in LCOV_EXCL_START-LCOV_EXCL_STOP comments or use LCOV_EXCL_LINE"
  echo "But this should happen only for well reasoned exceptions!"
  exit 1
fi
