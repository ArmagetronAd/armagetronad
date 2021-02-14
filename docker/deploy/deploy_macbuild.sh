#!/bin/bash

# update and push macOS build on GitHib

set +x

mv secrets/ssh ~/.ssh

rm -rf secrets/*

set -x

. ./version.sh || exit $?
. ./targets.sh || exit $?

dd=`dirname $0`

# get tarball URI, make sure it exists
TAR_FILENAME=${PACKAGE_NAME}-${PACKAGE_VERSION}.tbz
TAR_URI=${DOWNLOAD_URI_BASE_STAGING}${TAR_FILENAME}
./wait_for_upload.sh ${TAR_URI} || exit $?

trust_github || exit $?

BRANCH_BASE=${ZI_SERIES}

pushd macOS || exit $?

DEPLOY_BRANCH=deploy_${BRANCH_BASE}_${VERSION_SERIES}

# take current cached revision, put it into the deploy branch
git branch -d ${DEPLOY_BRANCH}  > /dev/null 2>&1 || true
git checkout -b ${DEPLOY_BRANCH} || exit $?

git remote add upstream ${MACOS_GIT} || exit $?

STEAM_PACKAGE_NAME=retrocycles
STEAM_PACKAGE_TITLE=Retrocycles

#PACKAGE_NAME_SUFFIX="`echo ${PACKAGE_NAME} | sed -e "s,.*-,-,"`"
#PACKAGE_TITLE_SUFFIX="`echo ${PACKAGE_TITLE} | sed -e "s,.* , ,"`"
#PROGTITLE_RC="${STEAM_PACKAGE_TITLE}${PACKAGE_TITLE_SUFFIX}"
#PROGNAME_RC="${STEAM_PACKAGE_NAME}${PACKAGE_NAME_SUFFIX}"

# write new definiton file
cat > INFO <<EOF
TARBALL=${TAR_URI}
VERSION=${PACKAGE_VERSION}
BRANCH=${ZI_SERIES}
PROGTITLE_RC="${STEAM_PACKAGE_TITLE}"
PROGNAME_RC="${STEAM_PACKAGE_NAME}"
UPLOAD=${UPLOAD_SCP_BASE_STAGING}${SF_DIR}/${PACKAGE_VERSION}/macOS
EOF

cat INFO

CHANGED=`git status --short -uno | sed -e "s/^ . //"`
if test -z "${CHANGED}"; then
    echo "Nothing updated."
    exit 0
fi

# commit and push
git commit . -m "Update to version ${PACKAGE_VERSION}" || exit $?
if ! test ${STAGING} == true; then
    git push --force --set-upstream upstream ${DEPLOY_BRANCH} || exit $?
fi
popd

# cleanup
rm -rf source upload secrets secrets.*

if test ${STAGING} == true; then
    # just staging, no actual build is done
    exit 0
fi

# wait for result
sleep 300
MACOS_DOWNLOAD=${DOWNLOAD_URI_BASE_STAGING}/macOS/
timeout=5
ERROR=1
while test ${timeout} -gt -1; do
    if ./wait_for_upload.sh ${MACOS_DOWNLOAD}mac_is_done.txt; then
        timeout=-2
        ERROR=0
    else
        ERROR=$?
    fi
    timeout=$(( ${timeout} - 1 ))
done
test ${ERROR} == 0 || exit ${ERROR}

mkdir upload
for ext in dmg macOS.zip; do
    for base in ${PACKAGE_NAME} ${PACKAGE_NAME}-dedicated; do #${STEAM_PACKAGE_NAME}; do
        FILE=${base}-${PACKAGE_VERSION}.${ext}
        curl ${MACOS_DOWNLOAD}${FILE} --fail --silent --show-error -L -o upload/${FILE} || exit $?
    done
done
