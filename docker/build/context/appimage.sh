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
cp appdir/usr/share/games/${PACKAGE_NAME}*/desktop/icons/48x48/*.png appdir/.DirIcon || exit $?

# lint according to https://github.com/AppImage/appimage.github.io#checklist-for-submitting-your-own-appimage
chmod +x pkg2appimage/appdir-lint.sh
desktop-file-validate appdir/*.desktop || exit $?

VALIDATE_ARG=""
if appstreamcli --help > /dev/null; then
    if test "`appstreamcli --version`" != "AppStream CLI tool version: 0.9.4"; then
        ./pkg2appimage/appdir-lint.sh ./appdir || exit $?
    else
        # old version does not support content_rating, transform to something compliant for validation
        # this case can be removed once we bump the base build system to Ubuntu 20.04 or 18.04
        cp -a appdir appdir_validate || exit $?
        find appdir_validate -name *.appdata.xml -exec sed -i \{\} -e s/content_rating/x-content_rating/ -e s/launchable/x-launchable/g \;
        ./pkg2appimage/appdir-lint.sh ./appdir_validate || exit $?
        rm -rf appdir_validate
        VALIDATE_ARG="-n"
    fi
fi

#   pack it up. The appimagetool package is available here:
#   https://github.com/AppImage/AppImageKit/releases

rm -f $1
rm -f appdir/*-handler.desktop
ARCH=${ARCH} appimagetool-${ARCH}.AppImage --appimage-extract-and-run ${VALIDATE_ARG} appdir $1 || exit $?

# test that the package runs with and without system libraries
./$1 --appimage-extract-and-run --version || exit $?
LD_LIBRARY_PATH="" LD_DEBUG_APP=true ./$1 --appimage-extract-and-run --version || exit $?

# comment out to inspect result for debug purposes
# rm -rf appdir
