#!/bin/bash

# pack portable application dir into AppImage
set -x

. ./version.sh

# determine arch suffix
ARCH=x86_64
if test `getconf LONG_BIT` -eq 32; then
  ARCH=i686
fi

# link icon
cp appdir/usr/share/games/${PACKAGE_NAME}*/desktop/icons/large/*.png appdir/.DirIcon || exit $?

# lint according to https://github.com/AppImage/appimage.github.io#checklist-for-submitting-your-own-appimage
chmod +x pkg2appimage/appdir-lint.sh
desktop-file-validate appdir/*.desktop || exit $?
./pkg2appimage/appdir-lint.sh ./appdir || exit $?

#   pack it up. The appimagetool package is available here:
#   https://github.com/AppImage/AppImageKit/releases

rm -f $1
ARCH=${ARCH} appimagetool-${ARCH}.AppImage --appimage-extract-and-run appdir $1 || exit $?

./$1 --appimage-extract --version || exit $?

# comment out to inspect result for debug purposes
rm -rf appdir
