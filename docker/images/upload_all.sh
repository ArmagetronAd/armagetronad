#!/bin/bash

# uploads all images

wd="`dirname $0`"
. ${wd}/prefer_podman.sh || exit $?
. ${wd}/epoch.sh || exit $?

#set -x

function store(){
    logfile=`tempfile 2>/dev/null` || logfile=`mktemp` || exit $?
    if ! docker push ${REGISTRY}$1:${EPOCH} --digestfile ${wd}/$1.digest; then
        docker push ${REGISTRY}$1:${EPOCH} | tee ${logfile} || exit $?
        grep "digest:" ${logfile} || exit 0
        grep "digest:" ${logfile} | sed -e "s/.*digest: //" -e "s/ .*//" > ${wd}/$1.digest
    fi
    rm -f ${wd}/$1.digest.local ${logfile}

    # bake digest into .gitlab-ci.yml
    sed -i ${wd}/../../.gitlab-ci.yml -e "s,image: \$CI_REGISTRY_IMAGE/$1.*,image: \$CI_REGISTRY_IMAGE/$1@`cat ${wd}/$1.digest`,"
}

function upload_image(){
    ${wd}/../scripts/ensure_image.sh $1
    wait
    store $1 &
}

upload_image armaroot_64
upload_image armawineblocks
upload_image armasteam_64
upload_image armabuild_64
upload_image armabuild_32
upload_image armadeb_64
upload_image armadeploy_64
upload_image armalpine_32
upload_image steamcmd
wait


