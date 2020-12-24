#!/bin/bash

# build a portable application dir
test -x fakerelease.sh && . fakerelease.sh
. version.sh || exit $?

set -x

SUFFIX=
cat serverclient | grep "server" > /dev/null && SUFFIX=-dedicated
make -C build install DESTDIR=`pwd`/appdir || exit $?
APPDIR=appdir PACKAGE=${PACKAGE_NAME}${SUFFIX} portable/build || exit $?

# validate
if appstreamcli --help > /dev/null; then
    if test "`appstreamcli --version`" != "AppStream CLI tool version: 0.9.4"; then
        appstreamcli validate-tree appdir || exit $?
    else
        # old version does not support content_rating, transform to something compliant for validation
        # this case can be removed once we bump the base build system to Ubuntu 20.04 or 18.04
        cp -a appdir appdir_validate || exit $?
        find appdir_validate -name *.appdata.xml -exec sed -i \{\} -e s/content_rating/x-content_rating/ -e s/launchable/x-launchable/g \;
        appstreamcli validate-tree appdir_validate || exit $?
        rm -rf appdir_validate
    fi
fi

# test with and without system libraries
appdir/AppRun --version || exit $?
LD_LIBRARY_PATH="" LD_DEBUG_APP=true appdir/AppRun --version || exit $?

# comment out to inspect result for debug purposes
rm -rf build source
