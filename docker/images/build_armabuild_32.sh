#!/bin/bash

# builds an ubuntu based server/client builder

wd="`dirname $0`"
${wd}/build_armabuild.sh i386/ubuntu:16.04 armabuild_32 --target armabuild


