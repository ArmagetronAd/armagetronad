#!/bin/bash

# removes all images

#set -x

wd="`dirname $0`"
sd="${wd}/../scripts"

. ${wd}/prefer_podman.sh || exit $?
. ${wd}/epoch.sh || exit $?

for E in ${EPOCH}; do
    for A in _64 _32 ""; do
	for image in \
        armabuild \
        armawineblocks \
        armalpine \
        armadeb \
        armaroot \
        armadeploy \
        ; do
	    docker rmi -f --no-prune ${REGISTRY}${image}${A}:${E}
	done
    done
done


