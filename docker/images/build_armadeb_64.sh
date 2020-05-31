#!/bin/bash

# builds everything required for debian package builds

wd="`dirname $0`"
${wd}/build_armabuild.sh amd64/ubuntu:16.04 armadeb_64 --target armadeb

