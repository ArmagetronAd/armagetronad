#!/bin/bash

# usage: build_image.sh <base docker image> <docker image name> <work directory>
# expected to be called from within the docker context
# with a prepared Docker.proto file only missing the first FROM line

set -x

sd=`dirname "$0"`
id="${sd}/../images"
${sd}/ensure_image.sh "$1" -d || exit $?

. ${id}/digest.sh "$1" || exit $?

touch ${id}/$2.digest.local || exit $?

BASE=$1
echo ${BASE} | grep ':' > /dev/null || BASE=${REGISTRY}$1${REFERENCE}

IMAGE=$2

pushd "$3" || exit $?

shift
shift
shift

lock=.lock.build_image
#cat ${lock}
while test -r ${lock} && ps -eo pid | grep "[\^ ]`cat ${lock}`\$"; do
    sleep 10
done
echo $$ > ${lock}

if echo FROM ${BASE} AS base > Dockerfile; then
    if cat Dockerfile.proto >> Dockerfile; then
	if docker image build . -t "${REGISTRY}${IMAGE}:${EPOCH}" $*; then
	    echo "Done!"
	fi
    fi
fi

rm -f ${lock}

popd
