#!/bin/bash

# update zeroinstall streams on the servers

set +x

mv secrets/ssh ~/.ssh

# import gpg keys; remove other secrets
EXIT=0
gpg --import secrets/pub.gpg || EXIT=$?
gpg --allow-secret-key-import --import secrets/sec.gpg || EXIT=$?
rm -rf secrets/*
test ${EXIT} = 0 || exit ${EXIT}

set -x

. ./version.sh || exit $?
. ./targets.sh || exit $?

dd=`dirname $0`

pushd zeroinstall

CHANGED=`git status --short -uno | sed -e "s/^ . //"`
if test -z "${CHANGED}"; then
    echo "Nothing updated."
    exit 0
fi

# remove all signatures
for f in ${CHANGED}; do
     0launch -o -c 'http://0install.net/2006/interfaces/0publish' $f -u || true
done

# commit and push
trust_gitlab
git pull || exit $?
git commit ${CHANGED} -m "Update ${ZI_SERIES} to version ${PACKAGE_VERSION}" || exit $?
if ! test ${STAGING} == true; then
    git push || exit $?
fi

# upload all files
CHANGED=`ls *.xml`
CHANGED_XML="${CHANGED}"

# sign XML files
for f in ${CHANGED_XML}; do
     0launch -o -c 'http://0install.net/2006/interfaces/0publish' $f -x || exit $?
done

# DEPLOY
trust_bugfarm
rsync ${CHANGED} ${UPLOAD_ZI_SCP} || exit $?
    
popd

# cleanup
rm -rf source upload secrets secrets.*

exit ${EXIT}
