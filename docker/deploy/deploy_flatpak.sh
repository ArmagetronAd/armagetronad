#!/bin/bash

# update and push flatpak definition

set +x

mv secrets/ssh ~/.ssh

rm -rf secrets/*

set -x

. ./version.sh || exit $?
. ./targets.sh || exit $?

dd=`dirname $0`

trust_gitlab || exit $?
git clone ${FP_GIT} flatpak || exit $?

BRANCH_BASE=${ZI_SERIES}

pushd flatpak || exit $?
git checkout ${BRANCH_BASE}_0.2.9_ci || exit $?
FILENAME=${PACKAGE_NAME}-${PACKAGE_VERSION}.tbz

# scary SED patch in new package source
SHA=`sha256sum ../upload/${FILENAME} | sed -e "s, .*,,"`
sed -i org.armagetronad.ArmagetronAdvanced.json -e \
"s~\\\"url\\\":.*armagetronad.*~\\\"url\\\": \\\"${DOWNLOAD_URI_BASE}${FILENAME}\\\",~" -e \
"s~\\\"sha256\\\":.*~\\\"sha256\\\": \\\"${SHA}\\\"~" || exit $?
git diff

CHANGED=`git status --short -uno | sed -e "s/^ . //"`
if test -z "${CHANGED}"; then
    echo "Nothing updated."
    exit 0
fi

# commit and push
git commit . -m "Update to version ${PACKAGE_VERSION}" || exit $?
if ! test ${STAGING} == true; then
    git push || exit $?
fi
popd

# cleanup
rm -rf source upload secrets secrets.*
