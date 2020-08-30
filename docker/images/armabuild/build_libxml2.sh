#!/bin/bash

# usage: build_libxml2.sh <name of liblxm2 directory>, e.g. libxml2-2.9.10

set +x
archive=$1

# if it already exists (Steam SDK), don't bother
test -r /usr/lib/libxml2.so && exit 0
test -r /usr/lib/x86_64-linux-gnu/libxml2.so && exit 0

# fetch
curl ftp://xmlsoft.org/libxml2/${archive}.tar.gz -o ${archive}.tar.gz

# unpack, configure, build, install, cleanup
tar -xzf ${archive}.tar.gz
rm ${archive}.tar.gz
cd ${archive}
./configure --without-icu --prefix=/usr/
make -j 5
make install
cd .. && rm -rf ${archive}
