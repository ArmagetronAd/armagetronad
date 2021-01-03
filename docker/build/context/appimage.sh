#!/bin/bash

# pack portable application dir into AppImage
set -x

# import gpg keys
SIGN_POSSIBLE=0
gpg --import pub.gpg || SIGN_POSSIBLE=$?
gpg --allow-secret-key-import --import sec.gpg || SIGN_POSSIBLE=$?
rm -rf *.gpg

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

#docs on updatable AppImages: https://github.com/AppImage/docs.appimage.org/blob/master/source/packaging-guide/optional/updates.rst
ZSYNC=`echo $1 | sed -e s,-${PACKAGE_VERSION},,`.zsync
SIGN=""
test ${SIGN_POSSIBLE} = 0 && SIGN=--sign
ARCH=${ARCH} appimagetool-${ARCH}.AppImage --appimage-extract-and-run ${VALIDATE_ARG} -u "zsync|https://download.armagetronad.org/appimage/${ZSYNC}" appdir $1 ${SIGN} || exit $?

# test that the package runs with and without system libraries
./$1 --appimage-extract-and-run --version || exit $?
LD_LIBRARY_PATH="" LD_DEBUG_APP=true ./$1 --appimage-extract-and-run --version || exit $?

# comment out to inspect result for debug purposes
# rm -rf appdir
