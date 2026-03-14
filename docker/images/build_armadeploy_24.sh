#!/bin/bash

# builds an deploy capable machine

wd="`dirname $0`"
${wd}/build_armabuild.sh amd64/ubuntu:24.04 armadeploy_24 --target armadeploy

# Version choice: LP_CREDENTIALS_FILE requires at least ubuntu 22.04
# we skip that so we do not need to update so soon again.
