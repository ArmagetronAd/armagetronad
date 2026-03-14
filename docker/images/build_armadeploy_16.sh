#!/bin/bash

# builds an deploy capable machine

wd="`dirname $0`"
${wd}/build_armabuild.sh amd64/ubuntu:16.04 armadeploy_16 --target armadeploy_nozero

# very old ubuntu system where lo-project-upload works with the old interface
# (the new interface from Ubuntu 22.04 does not support passing a series reliably,
# and 20.04 does nut support credential storing)
# 0launch, sadly, cannot be installed here, it does not work properly.
