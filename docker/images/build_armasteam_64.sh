#!/bin/bash

# builds a basic arma build environment based on alpine, 64 bit

wd="`dirname $0`"
. ${wd}/epoch.sh

${wd}/build_armabuild.sh registry.gitlab.steamos.cloud/steamrt/scout/sdk armasteam_64 --target armabuild_zthreadconfig

