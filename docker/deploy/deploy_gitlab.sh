#!/bin/bash

# push upload directory to gitlab release

. ./version.sh || exit $?
. ./targets.sh || exit $?
. CI_INFO_DEPLOY_GITLAB || exit $?

set -x

# gitlab only accepts NUMBER.NUMBER.NUMBER
# Replace all non-number sequences by dots
# pad numbers to two digits
# remove leading 0
# only leave the first three dots
GITLAB_VERSION=`echo ${PACKAGE_VERSION} | sed -e 's,[[:alpha:]_]\+,.,g' -e 's,\.\([0-9]\)\.,.0\1.,g' -e 's,\.\([0-9]\)\.,.0\1.,g' -e 's,\.\([0-9]\)\.,.0\1.,g' -e 's,^0.,0,' -e 's,\.,,3g'` || exit $?

if ${STAGING} == true; then
	exit 0
fi

function upload(){
	curl --header "JOB-TOKEN: $CI_JOB_TOKEN" --upload-file $1 "${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/packages/generic/${PACKAGE_NAME}/${GITLAB_VERSION}/`basename $1`"
}

EXIT=0
for f in upload/*.AppImage upload/*.exe upload/${PACKAGE_NAME}-${PACKAGE_VERSION}*.tbz upload/*source*.zip; do
	if test -r $f; then
		upload $f || EXIT=$?
	fi
done
rm -rf source upload ulb secrets ~/.ssh
exit ${EXIT}
