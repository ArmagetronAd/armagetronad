#!/bin/bash

# runs the yaml file through gitlab-runner

set -x

wd=`dirname $0`
ref=`git rev-parse HEAD`

# create and auto-revert commit
git -C ${wd}/../../ commit . -m "TEMP TO TEST YAML"
{ sleep 5; git -C ${wd}/../../ reset ${ref}; } &

# execute runner
pushd ${wd}/../../
gitlab-runner exec shell $*
popd

wait
