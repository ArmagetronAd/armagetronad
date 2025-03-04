#!/bin/bash

# builds an deploy capable machine

wd="`dirname $0`"
${wd}/build_armabuild.sh amd64/ubuntu:22.04 armadeploy_64 --target armadeploy


