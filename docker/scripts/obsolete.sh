#!/bin/bash

# returns an error if a later build pipeline is already running
# to be used in .gitlab-ci.yml as
# docker/scripts/obsolete.sh || exit 0

wget ${CI_SERVER_URL}/api/v4/projects/${CI_PROJECT_ID}/pipelines -O pipelines.json
EXIT=1

# remove any newlines
# add newlines between pipelines
# pick those from the same branch
# pick the first one
# check whether it's our run
tr -d '\n' < pipelines.json \
| sed -e 's/}[^{]*,[^}]*{/\n/g' \
| grep \"ref\":\"${CI_COMMIT_REF_NAME}\" \
| head -n 1 \
| grep -q \"id\":${CI_PIPELINE_ID} && EXIT=$?

if test ${EXIT} != 0; then
    echo "A later pipeline is already running."
fi

rm pipelines.json
exit ${EXIT}
