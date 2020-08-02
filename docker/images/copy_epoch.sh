#!/bin/bash

# usage: copy_epoch.sh <source> <target>
# re-tag epoch <soure> to <target>

set -x

source=$1
target=$2

test -z "${source}" && exit 1
test -z "${target}" && exit 1

wd="`dirname $0`"
. ${wd}/epoch.sh || exit $?

for image in \
    armawineblocks \
    armasteam_64 \
    armabuild_32 \
    armadeb_64 \
    armabuild_64 \
    armalpine_32 \
    armaroot_64 \
    armadeploy_64 \
    steamcmd \
    ; do
    docker tag ${REGISTRY}${image}:${source} ${REGISTRY}${image}:${target}
done
