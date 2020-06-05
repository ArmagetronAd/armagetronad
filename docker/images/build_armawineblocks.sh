#!/bin/bash

# builds everything

set -x

BASE_32=i386/alpine:3.7

wd="`dirname $0`"
bd="${wd}/context/wineblocks"
sd="${wd}/../scripts"
mkdir -p ${wd}/context
rm -rf ${bd}
cp -ax ${wd}/wineblocks ${bd}

. ${wd}/epoch.sh

# noninteractive setup begin
${sd}/download.sh ${bd}/download/codeblocks-setup1312.exe https://sourceforge.net/projects/codeblocks/files/Binaries/13.12/Windows/codeblocks-13.12mingw-setup.exe/download || exit 1
${sd}/download.sh ${bd}/download/nsis-setup304.exe https://sourceforge.net/projects/nsis/files/NSIS%203/3.04/nsis-3.04-setup.exe/download || exit 1
${sd}/download.sh ${bd}/download/winetricks https://raw.githubusercontent.com/Winetricks/winetricks/master/src/winetricks || exit 1
chmod +x ${bd}/download/winetricks || exit 1
${sd}/build_image.sh ${BASE_32} armabuild_wine_1 "${bd}" || exit 1

# interactive setup: wine and nsis
docker rm armadev
x11docker --name armadev --keepcache --user=RETAIN ${REGISTRY}armabuild_wine_1:${EPOCH} ./install.sh || exit 1
docker commit armadev ${REGISTRY}armawineblocks:${EPOCH} || exit
touch ${id}/armawineblocks.digest.local
docker rm armadev

#echo TEST installation:
#docker run -it -e DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix:ro -u docker --name="armadev4" armabuild_wine_3 bash
#docker rm armadev4

docker rmi -f ${REGISTRY}armabuild_wine_1:${EPOCH}

