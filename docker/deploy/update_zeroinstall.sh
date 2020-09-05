#!/bin/bash

# update zeroinstall stream git with new files

set -x
#ls
#ls secrets
ls upload

set +x

mv secrets/ssh ~/.ssh

# import gpg keys; remove other secrets
EXIT=0
gpg --import secrets/pub.gpg || EXIT=$?
gpg --allow-secret-key-import --import secrets/sec.gpg || EXIT=$?
rm -rf secrets/*
test ${EXIT} = 0 || exit ${EXIT}

dd=`dirname $0`

set -x

. ./version.sh || exit $?
. ./targets.sh || exit $?

ZEROVERSION=$(echo ${PACKAGE_VERSION} | sed -e "s,_,-,g" -e "s,-alpha-,-pre0.X," -e "s,-beta-,-pre1.X," -e "s,-rc-,-pre2.X," -e "s,\.Xz,.," -e "s,\.Xr,.," -e "s,\.X,.," )

case ${STAGING}+${ZI_SERIES}+${ZEROVERSION} in
    false+*+**)
    # feed determines default stability
	STABILITY=""
	;;
    true+stable+*)
    # staged release; shoud be > last RC (by version) but < last true release (by stability)
	STABILITY=testing
	;;
    true+*+*)
    # alpha/beta/other staging, go to lowest sensible stability
	STABILITY=developer
	;;
    *)
    # unknown, feed default makes the call
	STABILITY=""
	;;
esac

trust_gitlab || exit $?
git clone ${ZI_GIT} zeroinstall || exit $?
cp zeroinstall/*.gpg . || exit $?

# FeedLint requires this
export TERM=linux

function wait_for_upload(){
    URI="$1"

    # wait a bit for file to be available. SF and LP can take some minutes.
    timeout=10
    while test ${timeout} -gt -1; do
        if curl ${URI} --fail --silent --show-error > /dev/null; then
            return
        elif test ${timeout} -gt 0; then
            echo "Waiting for download ${URI}: ${timeout}"
            sleep 120
        fi
        timeout=$(( ${timeout} - 1 ))
    done

    # timed out, fail. The high level retry is going to loop around a couple of times.
    exit 1
}

function update_stream(){
    STREAM=$1
    FILE=$2
    MAIN=$3
    
    # nonexisting file is not an error
    test -r "${FILE}" || return 0
    
    URI=${DOWNLOAD_URI_BASE}`basename ${FILE}`

    wait_for_upload "${URI}"

    XML=zeroinstall/${PACKAGE_NAME_BASE}-${ZI_SERIES}-${STREAM}.xml
    test -f ${XML} || exit 1
    grep -q version=\"${ZEROVERSION}\" ${XML} && return 0

    STAB=""
    test -z "${STABILITY}" || STAB="--set-stability=${STABILITY}"

    0launch -o -c 'http://0install.net/2006/interfaces/0publish' \
	    ${XML} \
	    --add-version ${ZEROVERSION} \
	    --archive-url=${URI} \
	    --archive-file=${FILE} \
	    ${STAB} \
	    --set-main=${MAIN} \
	    --set-released=today -c -x || exit $?

    0launch -o -c 'http://0install.net/2007/interfaces/FeedLint.xml' -o \
	    ${XML} || exit $?
}

for f in upload/*client*win32.zip; do
    update_stream Windows $f ${PACKAGE_NAME}.exe
done
    
for f in upload/*server*win32.zip; do
    update_stream dedicated-Windows $f ${PACKAGE_NAME}_dedicated.exe
done

for f in upload/*client_32*.tbz; do
    update_stream Linux-i486 $f AppRun
done

for f in upload/*server_32*.tbz; do
    update_stream dedicated-Linux-i486 $f AppRun
done

for f in upload/*client_64*.tbz; do
    update_stream Linux-x86_64 $f AppRun
done

for f in upload/*server_64*.tbz; do
    update_stream dedicated-Linux-x86_64 $f AppRun
done

rm -rf source upload

exit ${EXIT}
