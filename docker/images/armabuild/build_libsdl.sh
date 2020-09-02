#!/bin/bash

# usage: $0 <name of library directory>, e.g. SDL-1.2.15

set -x
archive=$1

# if it already exists (Steam SDK), don't bother
test -d /usr/include/SDL && exit 0

# remove system provided libxml2
# rm -f /usr/lib/libxml2.so.2 /usr/lib/x86_64-linux-gnu/libxml2.so.2 /usr/lib/i386-linux-gnu/libxml2.so.2

# fetch
curl https://www.libsdl.org/release/${archive}.tar.gz -o ${archive}.tar.gz || exit $?

# unpack, configure, build, install, cleanup
tar -xzf ${archive}.tar.gz
rm ${archive}.tar.gz
cd ${archive}
patch -Np1 -i ../libsdl-1.2.15-const-xdata32.patch || exit $?
rm -f ../libsdl-1.2.15-const-xdata32.patch
CFLAGS=-Os ./configure --prefix=/usr/ || exit $?
make -j 5 || exit $?
make install || exit $?
cd .. && rm -rf ${archive}

