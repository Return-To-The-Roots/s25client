#!/bin/bash

set -euo pipefail

# Try to convert all source files to UTF-8 to detect encoding errors
# Execute in the project folder (s25client)
find . -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.tpp' -o -name '*.h' \) -exec iconv -f UTF-8 > /dev/null {} \;
