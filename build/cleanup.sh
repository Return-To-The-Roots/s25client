#!/bin/sh

SRCDIR=$PWD

echo "Cleaning up $PWD"
rm -vf CMakeCache.txt cmake_install.cmake Makefile install_manifest.txt prepareRelease.sh start.sh
rm -vrf CMakeFiles Testing bugle build_paths.h build_version.h build_version_defines.h postinstall.sh CTestTestfile.cmake
rm -vf *.bak *~ .DS_Store
# Symlinks
rm -vrf bin lib libexec share s25rttr test

# Unterverzeichnisse entfernen
for I in `grep -i ADD_SUBDIRECTORY ../CMakeLists.txt | cut -d '(' -f 2 \
          | cut -d ')' -f 1` ; do
	rm -vrf "$I"
done

exit 0
