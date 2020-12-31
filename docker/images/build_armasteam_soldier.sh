#!/bin/bash

# builds a basic arma build environment based on steam soldier

wd="`dirname $0`"
sd="${wd}/../scripts"
. ${wd}/epoch.sh
bd="${wd}/context/armasteam_soldier"
rm -rf ${bd}
cp -ax ${wd}/armasteam ${bd}

# apt update does not work for the steam runtime SDKs
sed -i -e 's/apt-get -y update &&//' ${bd}/Dockerfile.proto

# and the hacky update from Ubuntu 16.04 is also obsolete
sed -i -e 's/^FROM\|^COPY/# /' ${bd}/Dockerfile.proto

# SDK source: see https://gitlab.steamos.cloud/steamrt/scout/sdk
# see also https://github.com/ValveSoftware/steam-runtime#building-in-the-runtime

# fixed tag from https://gitlab.steamos.cloud/steamrt/soldier/sdk/container_registry/15
TAG=0.20201210.0
# or always-latest tag? Fixed tag is probably better.
# TAG=latest-steam-client-general-availability

SDK=registry.gitlab.steamos.cloud/steamrt/soldier/sdk:${TAG}
docker pull ${SDK}
${sd}/build_image.sh ${SDK} armasteam_soldier "${bd}" $* || exit $?
