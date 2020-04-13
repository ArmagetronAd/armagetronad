#!/bin/bash

# builds a basic arma build environment based on alpine, 64 bit

wd="`dirname $0`"
. ${wd}/epoch.sh

${wd}/../scripts/ensure_image.sh steamrt_scout_amd64
${wd}/build_armabuild.sh steamrt_scout_amd64 armasteam_64 --target armabuild_zthreadconfig

