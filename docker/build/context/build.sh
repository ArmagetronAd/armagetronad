#!/bin/bash

# do a build from source to build
test -x fakerelease.sh && . fakerelease.sh
. version.sh || exit $?

set -x

# read extra arguments to configure from context
extra_configure_args="`cat extra_configure_args 2>/dev/null || true`"

mkdir -p build
pushd build
progname="${PACKAGE_NAME}" progtitle="${PACKAGE_TITLE}" ../source/configure --prefix=/usr $extra_configure_args $@ || exit $?
nice -n 19 make -j `nproc` ci || exit $?
popd

