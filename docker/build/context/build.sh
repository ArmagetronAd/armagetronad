#!/bin/bash

# do a build from source to build
test -x fakerelease.sh && . fakerelease.sh
. version.sh || exit $?

set -x

mkdir -p build
pushd build
progname="${PACKAGE_NAME}" progtitle="${PACKAGE_TITLE}" ../source/configure --prefix=/usr/local $@ || exit $?
make -j `nproc` || exit $?
popd

