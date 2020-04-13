#!/bin/bash

# push upload directory to launchpad via lpaunchpad-lib

#set -x
#ls
#ls secrets
#ls upload

#set +x

# import gpg keys; remove all secrets except lp-credentials
mv secrets/lp-credentials .
EXIT=0
gpg --import secrets/pub.gpg || EXIT=$?
gpg --allow-secret-key-import --import secrets/sec.gpg || EXIT=$?
rm -rf secrets/*
test ${EXIT} = 0 || exit ${EXIT}
mv lp-credentials secrets/

set -x

. ./version.sh || exit $?
. ./targets.sh || exit $?

if test ${STAGING} == true; then
    echo "Just staging, no upload to Launchpad"
else
    EXIT=0
    for f in upload/*${PACKAGE_VERSION}*; do
	if test -r $f; then
	    ./lp-project-upload ${LP_PROJECT} ${LP_SERIES} ${LP_VERSION} $f || EXIT=$?
	fi
    done
fi

# cleanup
rm -rf source upload secrets secrets.*

exit ${EXIT}
