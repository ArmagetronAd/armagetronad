#!/bin/bash

# push upload directory to server via rsync/ssh
set -x

function upload(){
    . ./version.sh || return $?
    . ./targets.sh || return $?
    
    rm -rf ~/.ssh
    mv secrets/ssh ~/.ssh || return $?
    chmod 700 ~/.ssh || return $?
    chmod 400 ~/.ssh/id* || return $*
    
    # troubleshooting
    #ls -al ~/.ssh
    #ssh -V
    #ssh -v bazaaarmagetron,armagetronad@frs.sourceforge.net
    #return 1

    trust_bugfarm

    # upload versionless appimage into fixed directory for AppImageHub
    if ! test -z "${UPLOAD_SCP_APPIMAGE}"; then
        APPIMAGE_BASENAME=`echo ${PACKAGE_TITLE} | sed -e "s, ,,g"`
        for f in upload/${APPIMAGE_BASENAME}*; do
            target=`echo $f | sed -e s,upload/,, -e s,-${PACKAGE_VERSION},,`
            scp $f ${UPLOAD_SCP_APPIMAGE}${target} || return $?
        done
        # return 0
    fi

    # bulk upload all of the upload directory to a versioned subdirectory
    mkdir -p ulb || return $?
    rm -f upload/.tag
    mv upload ulb/${PACKAGE_VERSION} || return $?
    rsync -avP -e ssh ulb/ ${UPLOAD_SCP} || return $?
}

EXIT=0
upload || EXIT=$?
rm -rf source upload ulb secrets ~/.ssh
exit ${EXIT}
