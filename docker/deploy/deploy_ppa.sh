#!/bin/bash

# import gpg keys
EXIT=0
gpg --import pub.gpg || EXIT=$?
gpg --allow-secret-key-import --import sec.gpg || EXIT=$?
rm -rf *.gpg
test ${EXIT} = 0 || exit ${EXIT}

set -x

# push debian source package to ppa
. ./version.sh || exit $?
. ./targets.sh || exit $?

if test -z "${UBUNTU_PPA}"; then
    echo "No PPA, nothing to do."
    rm -rf *
    exit 0
fi

SERIES=$1
test -z ${SERIES} && SERIES=unstable

DEBIAN_VERSION=${DEBIAN_VERSION_BASE}~ppa1~${SERIES} || exit $?

CHANGES=${PACKAGE_NAME}_${DEBIAN_VERSION}_source.changes || exit $?
#ls ${CHANGES} || exit $?

dput ${UBUNTU_PPA} ${CHANGES} || exit $?

# cleanup
rm -rf *
