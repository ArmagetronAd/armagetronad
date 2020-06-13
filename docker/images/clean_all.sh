#!/bin/bash

# removes all images

#set -x

wd="`dirname $0`"
sd="${wd}/../scripts"

. ${wd}/epoch.sh

for E in ${EPOCH}; do
    for A in _64 _32 ""; do
	for image in armabuild armawineblocks steamrt_scout_amd64 armasteam armalpine armadeb armaroot armadeploy; do
	    docker rmi -f --no-prune ${REGISTRY}${image}${A}:${E}
	done
    done
done


