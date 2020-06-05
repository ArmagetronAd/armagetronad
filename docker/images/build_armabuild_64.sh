#!/bin/bash

# builds an ubuntu based server/client builder

wd="`dirname $0`"
${wd}/build_armabuild.sh amd64/ubuntu:16.04 armabuild_64 --target armabuild


