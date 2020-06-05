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
    
    
    mkdir -p ulb || return $?
    rm -f upload/.tag
    mv upload ulb/${PACKAGE_VERSION} || return $?
    trust_bugfarm
    rsync -avP -e ssh ulb/ ${UPLOAD_SCP} || return $?
}

EXIT=0
upload || EXIT=$?
rm -rf source upload ulb secrets ~/.ssh
exit ${EXIT}
