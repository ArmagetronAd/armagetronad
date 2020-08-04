#!/bin/bash

# usage: build_libxml2.sh <name of liblxm2 directory>, e.g. libxml2-2.9.10

set -x
archive=$1

# if it already exists (Steam SDK), don't bother
test -r /usr/lib/libxml2.a && exit 0
test -r /usr/lib/x86_64-linux-gnu/libxml2.a && exit 0

# remove system provided libxml2
rm -f /usr/lib/libxml2.so.2 /usr/lib/x86_64-linux-gnu/libxml2.so.2 /usr/lib/i386-linux-gnu/libxml2.so.2

# fetch
curl ftp://xmlsoft.org/libxml2/${archive}.tar.gz -o ${archive}.tar.gz || exit $?

# unpack, configure, build, install, cleanup
tar -xzf ${archive}.tar.gz
rm ${archive}.tar.gz
cd ${archive}
CFLAGS=-Os ./configure --without-icu --without-python --prefix=/usr/ || exit $?
make -j 5 || exit $?
make install || exit $?
cd .. && rm -rf ${archive}

