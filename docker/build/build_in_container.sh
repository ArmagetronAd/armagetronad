#!/bin/bash

# Usage: build_in_container.sh <base_image> <result> <command>
# Takes <base_image> as base, copies current directory into a build directory,
# runs <commmand>, copies resulting build directory to ./<result>/

base=$1
shift
result=$1
shift

if test -d ${result}; then
    echo ${result} already exists.
    exit 1
fi

set -x

# make sure base image exists
wd=`dirname $0`
${wd}/../scripts/ensure_image.sh "${base}" -d
. ${wd}/../images/digest.sh "${base}" || exit $?

# start daemon on demand
docker info > /dev/null || { systemctl --user start docker; sleep 5; }

dockeruser=`docker inspect --format '{{.Config.User}}' ${REGISTRY}${base}${REFERENCE}` || exit $?
home=/home/${dockeruser}
if test -z "$dockeruser";then
    dockeruser=root
    home=/root
fi

rm -f exiterror

# create dockerfile on the fly
tmp=`mktemp`
container_name=image_${PPID}_`basename ${tmp} | tr '[:upper:]' '[:lower:]'`
function build(){
docker build --no-cache -t ${container_name} -f - . <<EOF
FROM ${REGISTRY}${base}${REFERENCE}
RUN mkdir -p ${home}/BUILDDIR
COPY --chown=${dockeruser} ./ ${home}/BUILDDIR
WORKDIR ${home}/BUILDDIR
RUN $@ || (echo \$? > exiterror)
EOF
}

build $@ || exit $?

# extract output from container
function extract(){
    container_id=$(docker run -it -d ${container_name}) || return $?
    docker cp ${container_id}:${home}/BUILDDIR ${result} || return $?
    docker rm -f ${container_id} || true
    docker rmi -f ${container_name} || true
    test -r ${result}/exiterror && return `cat ${result}/exiterror`
    return 0
}

# on failure, remove output
if ! extract; then
    rm -rf ${tmp}
    rm -rf ${result}.error
    mv ${result} ${result}.error
    exit 1
fi

rm -rf ${tmp}

exit 0

# save disk space by deduplicating source and destination
set +x
function link_identical_files(){
    while read file; do
        if test -r "${result}/${file}"; then
            if diff "${file}" "${result}/${file}" > /dev/null; then
                echo cp -lf "${file}" ...
                cp -lf "${file}" "${result}/${file}"
            else
                echo NOT "${file}", differs
            fi
        fi
    done
}
find . -type f | link_identical_files || exit $?
set -x

