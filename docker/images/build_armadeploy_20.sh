#!/bin/bash

# builds an deploy capable machine

wd="`dirname $0`"
${wd}/build_armabuild.sh amd64/ubuntu:20.04 armadeploy_20 --target armadeploy

# version choice: Ubuntu 20.04 does not yet have Python 3.9, where
# base64.encodestring was removed in (with good reason), but
# 0publish still depends on it.
# butler from itch.io no longer works on 16.04, but it works here.
