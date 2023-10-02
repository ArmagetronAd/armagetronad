#!/bin/bash

# bake all image digests into gitlab CI

wd="`dirname $0`"
. ${wd}/prefer_podman.sh || exit $?
. ${wd}/epoch.sh || exit $?

#set -x

function bake(){
    # bake digest into .gitlab-ci.yml
    sed -i ${wd}/../../.gitlab-ci.yml -e "s,image: \$CI_REGISTRY_IMAGE/$1.*,image: \$CI_REGISTRY_IMAGE/$1@`cat ${wd}/$1.digest`,"
}

bake armaroot_64
bake armawineblocks
bake armasteam_64
bake armabuild_64
bake armabuild_32
bake armadeb_64
bake armadeploy_64
bake armalpine_32
bake steamcmd
wait


