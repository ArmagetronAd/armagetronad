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

# start X server suitable for docker (https://github.com/mviereck/x11docker/wiki/docker-build-with-interactive-GUI)
x11docker --xephyr --printenv --xoverip --no-auth --display=30 | grep --line-buffered DISPLAY > ${sd}/.cache/display.txt &
sleep 2
#cat ${sd}/.cache/display.txt
# have the build process connect to it
${sd}/build_image.sh ${BASE_32} armawineblocks${EPIC} "${bd}" --network=host --build-arg `sed < ${sd}/.cache/display.txt -e "s/ / --build-arg /g"` || exit 1

wait

#echo TEST installation:
#docker run -it -e DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix:ro -u docker --name="armadev4" armawineblocks:${EPIC} bash
#docker rm armadev4

#${REGISTRY}armawineblocks:${EPOCH}
