#!/bin/bash

# builds the steam build system

wd="`dirname $0`"
sd="${wd}/../scripts"
. ${wd}/epoch.sh
bd="${wd}/context/armasteam_64"
rm -rf ${bd}
cp -ax ${wd}/armasteam ${bd}

${wd}/../scripts/ensure_image.sh steamrt_scout_amd64
#${wd}/../scripts/ensure_image.sh armabuild_64
${sd}/build_image.sh steamrt_scout_amd64 armasteam_64 "${bd}" $*

