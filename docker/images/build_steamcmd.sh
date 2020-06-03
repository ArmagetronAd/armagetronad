#!/bin/bash

# builds a steamcmd runner
# usage: $0 <base image name> <target image name>

set -x

bin=`echo $1 | sed -e "s,/,_,g"`
wd="`dirname $0`"
bd="${wd}/context/steamcmd"
sd="${wd}/../scripts"
mkdir -p ${wd}/context
rm -rf ${bd}
cp -ax ${wd}/steamcmd ${bd} || exit $?

${sd}/build_image.sh cm2network/steamcmd:latest steamcmd "${bd}"


