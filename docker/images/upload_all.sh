#!/bin/bash

# uploads all images

wd="`dirname $0`"
. ${wd}/prefer_podman.sh || exit $?
. ${wd}/epoch.sh || exit $?

#set -x

function store(){
    logfile=`tempfile 2>/dev/null` || logfile=`mktemp` || exit $?
    if docker push --help | grep digestfile > /dev/null; then
        docker push ${REGISTRY}$1:${EPOCH} --digestfile ${wd}/$1.digest
    else
        docker push ${REGISTRY}$1:${EPOCH} | tee ${logfile} || exit $?
        grep "digest:" ${logfile} || exit 0
        grep "digest:" ${logfile} | sed -e "s/.*digest: //" -e "s/ .*//" > ${wd}/$1.digest
    fi
    rm -f ${wd}/$1.digest.local ${logfile}

    # bake digest into .gitlab-ci.yml
    sed -i ${wd}/../../.gitlab-ci.yml -e "s,image: \$CI_REGISTRY_IMAGE/$1.*,image: \$CI_REGISTRY_IMAGE/$1@`cat ${wd}/$1.digest`,"
}

function upload_image(){
    image=$1
    shift
    ${wd}/../scripts/ensure_image.sh ${image} $@ || exit $?
    wait
    store ${image} &
}

upload_image armabuild_64 $@ || exit $?
upload_image armaroot_64 $@ || exit $?
upload_image armawineblocks $@ || exit $?
upload_image armasteam_64 $@ || exit $?
upload_image armadeb_64 $@ || exit $?
upload_image armadeploy_64 $@ || exit $?
#upload_image steamcmd $@ || exit $?

upload_image armabuild_32 $@ || exit $?

#upload_image armalpine_32 $@ || exit $?
wait


