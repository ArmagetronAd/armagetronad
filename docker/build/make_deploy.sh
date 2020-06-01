#!/bin/bash

# Helper script for deployment, used form (un)staged_% make targets
# Usage: make_deploy.sh <target to make>
# usually, <target to make> will be 'deploy'

set -x

EXIT=0

# first make run
if ! ${MAKE} $*; then
    # deployment occasionally fails due to network problems, wait a bit and try again
    sleep 300
    if ! ${MAKE} $*; then
	sleep 900
	# last run without fancy arguments
	make $* || EXIT=$?
    fi
fi

# reset deployment targets
rm -f ../deploy/targets.sh
${MAKE} ../deploy/targets.sh

# communicate final make return
exit ${EXIT}


