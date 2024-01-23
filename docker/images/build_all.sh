#!/bin/bash

# builds all images

set -x

wd="`dirname $0`"
sd="${wd}/../scripts"
${sd}/ensure_image.sh armawineblocks $@ || exit $?
${sd}/ensure_image.sh armasteam_64 $@ || exit $?
${sd}/ensure_image.sh armabuild_64 $@ &
wait || exit $?
${sd}/ensure_image.sh armadeb_64 $@ &
${sd}/ensure_image.sh armalpine_32 $@ || exit $?
${sd}/ensure_image.sh armaroot_64 $@ || exit $?
${sd}/ensure_image.sh armadeploy_64 $@ || exit $?
${sd}/ensure_image.sh steamcmd $@ || exit $?
${sd}/ensure_image.sh armabuild_32 $@ || exit $?
wait || exit $?

