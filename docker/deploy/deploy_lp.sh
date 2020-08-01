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

mv upload/RELEASENOTES.txt . || exit $?
mv upload/PATCHNOTES.txt . || exit $?
rm -f upload/*.md upload/*.html 

if test ${STAGING} == true; then
    echo "Just staging, no upload to Launchpad"
else
    EXIT=0
    for f in upload/*${PACKAGE_VERSION}* upload/*.txt; do
	if test -r $f; then
	    if ! ./lp-project-upload ${LP_PROJECT} ${LP_SERIES} ${LP_VERSION} $f PATCHNOTES.txt RELEASENOTES.txt; then
            sleep 10
            if ! ./lp-project-upload ${LP_PROJECT} ${LP_SERIES} ${LP_VERSION} $f PATCHNOTES.txt RELEASENOTES.txt; then
                sleep 10
                if ! ./lp-project-upload ${LP_PROJECT} ${LP_SERIES} ${LP_VERSION} $f PATCHNOTES.txt RELEASENOTES.txt; then
                    sleep 10
                    ./lp-project-upload ${LP_PROJECT} ${LP_SERIES} ${LP_VERSION} $f  PATCHNOTES.txt RELEASENOTES.txt || EXIT=$?
                fi
            fi
        fi
	fi
    done
fi

# cleanup
rm -rf source upload secrets secrets.*

exit ${EXIT}
