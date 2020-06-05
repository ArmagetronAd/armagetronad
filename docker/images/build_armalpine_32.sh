#!/bin/bash

# builds a basic arma build environment based on alpine, 64 bit

wd="`dirname $0`"
${wd}/build_armalpine.sh i386/alpine:3.7 armalpine_32


