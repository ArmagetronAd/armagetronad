#!/bin/bash

# Builds sources at $1 into docker image, pushes it to repository
# only version-tags stable enough builds
# cwd needs to be the build directory

. ./context/version.sh || exit $?
. ../deploy/targets.sh || exit $?

REGISTRY=${CI_REGISTRY_IMAGE:-registry.gitlab.com/armagetronad/armagetronad}
DI_TAG=${PACKAGE_NAME}:${PACKAGE_VERSION}

function tag()
{
    docker tag ${DI_TAG} $1 || exit $?
    docker push $1 || exit $?
}

set -x
docker build -t ${DI_TAG} "$1" || exit $?
if test "${ZI_SERIES}" = "stable" || test "${ZI_SERIES}" = "rc"; then
    tag ${REGISTRY}/${DI_TAG} || exit $?
fi
tag ${REGISTRY}/${PACKAGE_NAME} || exit $?
docker image rm ${DI_TAG} || exit $?
