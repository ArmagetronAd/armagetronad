#!/bin/bash

# Builds sources at $1 into docker image, pushes it to repository
# only version-tags stable enough builds
# cwd needs to be the build directory

. ./context/version.sh || exit $?
. ../deploy/targets.sh || exit $?

# register at repository
set +x
if test x${CI_COMMIT_REF_PROTECTED} = xtrue && ! test -z "${CI_REGISTRY_PASSWORD}" && ! test -z "${CI_REGISTRY_USER}" && ! test -z "${CI_REGISTRY}"; then
	echo ${CI_REGISTRY_PASSWORD} | docker login -u ${CI_REGISTRY_USER} --password-stdin ${CI_REGISTRY} 2>&1 | grep -v WARNING\|credential || exit $?
else
	CI_COMMIT_REF_PROTECTED=false
fi

if ${STAGING} == true; then
	NAME=${PACKAGE_NAME}-staging
else
	NAME=${PACKAGE_NAME}
fi

REGISTRY=${CI_REGISTRY_IMAGE:-registry.gitlab.com/armagetronad/armagetronad}
DI_TAG=${NAME}:${PACKAGE_VERSION}
CACHE=${REGISTRY}/cache:${PACKAGE_NAME}
BASE_CACHE=${REGISTRY}/cache:armagetronad

# tag a release; upload if we can
function tag()
{
	docker tag ${DI_TAG} $1 || exit $?
	if test x${CI_COMMIT_REF_PROTECTED} = xtrue; then
		docker push $1 || exit $?
	fi
}

# tag a cache; don't upload if we're deploying, there is nothing new to store then
function tag_cache()
{
	docker tag ${DI_TAG} $1 || exit $?
	if ! test -z "${DOCKER_NODEPLOY}" && test x${CI_COMMIT_REF_PROTECTED} = xtrue; then
		docker push $1 || exit $?
	fi
}

BUILDARGS="$1"
PAST_CACHE_ARGS=""

# build to target $1 using the repository-stored cache
function build_cached()
{
	TARGET=$1

	CACHE_ARGS="--cache-from ${CACHE}-${TARGET} \
		--cache-from ${BASE_CACHE}-${TARGET}"

	set -x
	docker pull ${CACHE}-${TARGET} || \
	docker pull ${BASE_CACHE}-${TARGET} || \
	CACHE_ARGS=""

	docker build --pull --target ${TARGET}\
		-t ${DI_TAG} ${PAST_CACHE_ARGS} ${CACHE_ARGS} ${BUILDARGS} || exit $?
	tag_cache ${CACHE}-${TARGET}
	PAST_CACHE_ARGS="--cache-from ${CACHE}-${TARGET} ${PAST_CACHE_ARGS}"
}

build_cached build
build_cached runtime

# final build
docker build --pull \
	-t ${DI_TAG} ${PAST_CACHE_ARGS} ${BUILDARGS} || exit $?

if test -z "${DOCKER_NODEPLOY}"; then
	if test "${ZI_SERIES}" = "stable" || test "${ZI_SERIES}" = "rc"; then
		tag ${REGISTRY}/${DI_TAG} || exit $?
	fi
	tag ${REGISTRY}/${NAME} || exit $?
else
	docker run --rm ${DI_TAG} --version || exit $?
fi

docker image rm ${DI_TAG} || exit $?
