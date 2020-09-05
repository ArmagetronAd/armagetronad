#!/bin/bash

# pack portable application dir into AppImage
set -x

. ./version.sh

# determine arch suffix
ARCH=x86_64
if test `getconf LONG_BIT` -eq 32; then
  ARCH=i686
fi

# copy icon
cp -l appdir/usr/local/share/games/${PACKAGE_NAME}*/desktop/icons/large/${PACKAGE_NAME}*.png appdir/

#   pack it up. The appimagetool package is available here:
#   https://github.com/AppImage/AppImageKit/releases

rm -f $1
ARCH=${ARCH} appimagetool-${ARCH}.AppImage --appimage-extract-and-run appdir $1 || exit $?

# test that the package runs with and without system libraries
./$1 --appimage-extract --version || exit $?
LD_LIBRARY_PATH="" LD_DEBUG_APP=true ./$1 --appimage-extract --version || exit $?

# comment out to inspect result for debug purposes
rm -rf appdir
