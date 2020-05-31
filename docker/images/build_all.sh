#!/bin/bash

# builds all images

set -x

wd="`dirname $0`"
sd="${wd}/../scripts"
${sd}/ensure_image.sh armawineblocks $*
${sd}/ensure_image.sh armasteam_64 $*
${sd}/ensure_image.sh armabuild_64 $* &
${sd}/ensure_image.sh armabuild_32 $*
wait
${sd}/ensure_image.sh armadeb_64 $* &
${sd}/ensure_image.sh armalpine_32 $*
${sd}/ensure_image.sh armaroot_64 $*
${sd}/ensure_image.sh armadeploy_64 $*
${sd}/ensure_image.sh steamcmd $*
wait

