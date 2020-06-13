#!/bin/bash

# do a simple test server build
test -x fakerelease.sh && . fakerelease.sh

mkdir build
cd build
../source/configure "$*"
make

