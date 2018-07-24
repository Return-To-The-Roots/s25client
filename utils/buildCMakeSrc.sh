#!/bin/sh

files=$(find "$1" -maxdepth 1 -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.tpp' -o -name '*.h' \))
for file in $files; do
    echo $file
done