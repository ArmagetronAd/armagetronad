#!/bin/bash

# uploads all images

wd="`dirname $0`"
. ${wd}/epoch.sh || exit $?

#set -x

function store(){
    logfile=`tempfile`
    docker push ${REGISTRY}$1:${EPOCH} | tee ${logfile}
    grep "digest:" ${logfile} || exit 0
    grep "digest:" ${logfile} | sed -e "s/.*digest: //" -e "s/ .*//" > ${wd}/$1.digest
    rm -f ${wd}/$1.digest.local ${logfile}
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

# bake digest into .gitlab-ci.yml
sed -i ${wd}/../../.gitlab-ci.yml -e "s,^image: ${REGISTRY}armaroot_64.*,image: ${REGISTRY}armaroot_64@`cat ${wd}/armaroot_64.digest`," -e "s,image: ${REGISTRY}armadeploy_64.*,image: ${REGISTRY}armadeploy_64@`cat ${wd}/armadeploy_64.digest`,"

