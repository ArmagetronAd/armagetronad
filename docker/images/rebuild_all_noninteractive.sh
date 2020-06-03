#!/bin/bash

# rebuilds and uploads those images that don't require interaction

wd=$(dirname $0)
for f in ${wd}/build_*_32.sh ${wd}/build_*_64.sh; do
    echo $f
    ./$f || exit $?
done

${wd}/upload_all.sh
