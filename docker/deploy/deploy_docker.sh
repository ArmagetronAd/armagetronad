#!/bin/bash

# Builds sources at $1 into docker image, pushes it to repository
# only version-tags stable enough builds
# cwd needs to be the build directory

. ./context/version.sh || exit $?
. ../deploy/targets.sh || exit $?


if ${STAGING} == true; then
	NAME=${PACKAGE_NAME}-staging
else
	NAME=${PACKAGE_NAME}
fi

REGISTRY=${CI_REGISTRY_IMAGE:-registry.gitlab.com/armagetronad/armagetronad}
DI_TAG=${NAME}:${PACKAGE_VERSION}
CACHE=${REGISTRY}/cache:${PACKAGE_NAME}
BASE_CACHE=${REGISTRY}/cache:armagetronad

function tag()
{
	docker tag ${DI_TAG} $1 || exit $?
	if test x${CI_COMMIT_REF_PROTECTED} = xtrue; then
		docker push $1 || exit $?
	fi
}

BUILDARGS="$1"
PAST_CACHE_ARGS=""
function build()
{
	TARGET=$1

	CACHE_ARGS="--cache-from ${CACHE}-${TARGET} \
		--cache-from ${BASE_CACHE}-${TARGET}"

	set -x
	docker pull ${CACHE}-${TARGET} || \
	docker pull ${BASE_CACHE}-${TARGET} || \
	CACHE_ARGS=""

	docker build --target ${TARGET}\
		-t ${DI_TAG} ${CACHE_ARGS} ${PAST_CACHE_ARGS} ${BUILDARGS} || exit $?
	tag ${CACHE}-${TARGET} || exit $?
	PAST_CACHE_ARGS="${PAST_CACHE_ARGS} --cache-from ${CACHE}-${TARGET}"
}

build build
build runtime
#build run_server

# final build
docker build \
	-t ${DI_TAG} ${PAST_CACHE_ARGS} ${BUILDARGS} || exit $?

if test -z "${DOCKER_NODEPLOY}"; then
	if test "${ZI_SERIES}" = "stable" || test "${ZI_SERIES}" = "rc"; then
		tag ${REGISTRY}/${DI_TAG} || exit $?
	fi
	tag ${REGISTRY}/${NAME} || exit $?
fi

docker image rm ${DI_TAG} || exit $?
