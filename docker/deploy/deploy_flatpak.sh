#!/bin/bash

# update and push flatpak definition

set +x

mkdir -p ~/.ssh
mv secrets/ssh/* ~/.ssh/

rm -rf secrets/*

set -x

. ./version.sh || exit $?
. ./targets.sh || exit $?

dd=`dirname $0`

trust_gitlab || exit $?
ls -alt ~/.ssh
git clone --recursive ${FP_GIT} flatpak || exit $?

BRANCH_BASE=${ZI_SERIES}

pushd flatpak || exit $?

# go back to last human edit
git checkout ${BRANCH_BASE}_${VERSION_SERIES}_ci || exit $?
git reset origin/${BRANCH_BASE}_${VERSION_SERIES} --hard || exit $?
git submodule update --checkout || exit $?

FILENAME=${PACKAGE_NAME}-${PACKAGE_VERSION}.tbz

# mildly scary SED patch in new package source
SHA=`sha256sum ../upload/${FILENAME} | sed -e "s, .*,,"`
sed -i org.armagetronad.ArmagetronAdvanced.yml -e \
"s~url:.*armagetronad.*~url: ${DOWNLOAD_URI_BASE}${FILENAME}~" -e \
"s~sha256:.*~sha256: ${SHA}~" || exit $?
git diff

CHANGED=`git status --short -uno | sed -e "s/^ . //"`
if test -z "${CHANGED}"; then
    echo "Nothing updated."
    exit 0
fi

# commit and push
git commit . -m "Update to version ${PACKAGE_VERSION}" || exit $?
if ! test ${STAGING} == true; then
    git push --force || exit $?
else
	git log -p -2
fi
popd

# cleanup
rm -rf source upload secrets secrets.*
