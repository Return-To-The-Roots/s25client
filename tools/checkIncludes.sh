#!/bin/sh

# Run IWYU (Include-What-You_Use) on the current build
# Execute in the build folder (e.g. s25client/build)
# Optional first argument: Folder containing iwyu_tool.py

iwyuBinary=`which include-what-you-use`

if [ -z "$iwyuBinary" ]; then
    echo "include-what-you-use not found in your path. Install and include it in your PATH variable"
    exit 1
fi

if [ $# -gt 0 ]; then
    iwyuDir=$1
else
    iwyuDir=`dirname "$iwyuBinary"`
fi

iwyuTool="$iwyuDir/iwyu_tool.py"

if [ ! -f "$iwyuTool" ]; then
    echo "iwyu tool not found at $iwyuTool. Please place it there or pass its directory to this script as the first param"
    exit 1
fi

cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .

if [ $? -ne 0 ]; then
    echo "Error during cmake -> exit"
    exit 1
fi

$iwyuTool -p .
