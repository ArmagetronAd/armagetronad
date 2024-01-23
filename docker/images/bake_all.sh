#!/bin/bash

# bake all image digests into gitlab CI

wd="`dirname $0`"
. ${wd}/prefer_podman.sh || exit $?
. ${wd}/epoch.sh || exit $?

set -x

function bake(){
    # bake digest into .gitlab-ci.yml
    sed -i ${wd}/../../.gitlab-ci.yml -e "s,image: \$CI_REGISTRY_IMAGE/$1.*,image: \$CI_REGISTRY_IMAGE/$1@`cat ${wd}/$1.digest`,"
}

for f in `ls ${wd}/*.digest`; do
    bake `echo $f | sed -e s,.*/,,g -e s,\.digest,,`
done


