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
# cp -l appdir/usr/share/games/${PACKAGE_NAME}*/desktop/icons/large/*.png appdir/${PACKAGE_NAME}.png || exit $?

#   pack it up. The appimagetool package is available here:
#   https://github.com/AppImage/AppImageKit/releases

rm -f $1
ARCH=${ARCH} appimagetool-${ARCH}.AppImage --appimage-extract-and-run appdir $1 || exit $?

./$1 --appimage-extract --version || exit $?

# comment out to inspect result for debug purposes
rm -rf appdir
