#!/bin/bash

# build a portable application dir
test -x fakerelease.sh && . fakerelease.sh
. version.sh || exit 1

set -x

SUFFIX=
cat serverclient | grep "server" > /dev/null && SUFFIX=-dedicated
make -C build install DESTDIR=`pwd`/appdir || exit
APPDIR=appdir PACKAGE=${PACKAGE_NAME}${SUFFIX} portable/build || exit

# comment out to inspect result for debug purposes
rm -rf build source
