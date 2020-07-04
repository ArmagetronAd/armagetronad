#!/bin/bash

# build a portable application dir
test -x fakerelease.sh && . fakerelease.sh
. version.sh || exit $?

set -x

SUFFIX=
cat serverclient | grep "server" > /dev/null && SUFFIX=-dedicated
make -C build install DESTDIR=`pwd`/appdir || exit $?
APPDIR=appdir PACKAGE=${PACKAGE_NAME}${SUFFIX} portable/build || exit $?

# test with and without system libraries
appdir/AppRun --version || exit $?
LD_LIBRARY_PATH="" LD_DEBUG_APP=true appdir/AppRun --version || exit $?

# comment out to inspect result for debug purposes
rm -rf build source
