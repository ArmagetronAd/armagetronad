#!/bin/bash

# builds a basic arma build environment based on steam scout, 64 bit

wd="`dirname $0`"
. ${wd}/epoch.sh

if test ${EPOCH} != 029_1; then
    # for future updates: no more need for building with build_steamrt_scout_amd64.sh
    # SDK source: see https://gitlab.steamos.cloud/steamrt/scout/sdk
    # see also https://github.com/ValveSoftware/steam-runtime#building-in-the-runtime

    # fixed tag from https://gitlab.steamos.cloud/steamrt/scout/sdk/container_registry/11
    TAG=0.20201203.1
    # or always-latest tag? Fixed tag is probably better.
    # TAG=latest-steam-client-general-availability

    SDK=registry.gitlab.steamos.cloud/steamrt/scout/sdk:${TAG}
    docker pull ${SDK}
    ${wd}/build_armabuild.sh ${SDK} armasteam_64 --target armabuild_zthreadconfig || exit $?
else
    # old way relying on cobbling together a local image from a tar
    ${wd}/../scripts/ensure_image.sh steamrt_scout_amd64 || exit $?
    ${wd}/build_armabuild.sh steamrt_scout_amd64 armasteam_64 --target armabuild_zthreadconfig || exit $?
fi
