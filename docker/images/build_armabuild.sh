#!/bin/bash

# builds a basic arma build environment based on alpine
# usage: $0 <base image name> <target image name>

set -x

bin=`echo $1 | sed -e "s,/,_,g"`
wd="`dirname $0`"
bd="${wd}/context/armabuild_${bin}_$2"
sd="${wd}/../scripts"
mkdir -p ${wd}/context
rm -rf ${bd}
cp -ax ${wd}/armabuild ${bd}

#${sd}/download.sh ${bd}/download/ZThread-2.3.2.tar.gz https://sourceforge.net/projects/zthread/files/ZThread/2.3.2/ZThread-2.3.2.tar.gz/download
#${sd}/download.sh ${bd}/download/zthread.patch.bz2 https://forums3.armagetronad.net/download/file.php?id=9628

if echo $2 | grep 64 > /dev/null; then
    ${sd}/download.sh ${bd}/appimage/appimagetool-x86_64.AppImage https://github.com/AppImage/AppImageKit/releases/download/12/appimagetool-x86_64.AppImage || exit 1
else
    ${sd}/download.sh ${bd}/appimage/appimagetool-i686.AppImage https://github.com/AppImage/AppImageKit/releases/download/12/appimagetool-i686.AppImage || exit 1
fi
chmod 755 ${bd}/appimage/*

BASE="$1"
IMAGE="$2"
shift
shift

${sd}/build_image.sh "${BASE}" "${IMAGE}" "${bd}" $*
exit $?

