#!/bin/bash

# builds the root environment with docker and selected tools

wd="`dirname $0`"
${wd}/build_armabuild.sh amd64/ubuntu:16.04 armaroot_64 --target armaroot_alpine
exit $?

bin=`echo $1 | sed -e "s,/,_,g"`
bd="${wd}/context/armaroot_${bin}_$2"
sd="${wd}/../scripts"
mkdir -p ${wd}/context
rm -rf ${bd}
cp -ax ${wd}/armaroot ${bd} || exit $?

${sd}/build_image.sh docker:19.03.0 armaroot_64 "${bd}"


