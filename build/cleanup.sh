#!/bin/sh

SRCDIR=$PWD

echo "Cleaning up $PWD"
rm -vf CMakeCache.txt cmake_install.cmake Makefile install_manifest.txt prepareRelease.sh
rm -vrf CMakeFiles Testing bugle build_paths.h build_version.h build_version_defines.h postinstall.sh CTestTestfile.cmake
rm -vf *.bak *~ .DS_Store
# Symlinks
rm -vf bin share s25rttr

# Unterverzeichnisse entfernen
for I in `grep ADD_SUBDIRECTORY ../CMakeLists.txt | cut -d '(' -f 2 \
          | cut -d ')' -f 1` ; do
	rm -vrf "$I"
done

exit 0
