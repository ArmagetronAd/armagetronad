#!/bin/bash

# usage: $0 <name of library directory>, e.g. SDL_mixer-1.2.12
set -x
archive=$1

# if it already exists (Steam SDK), don't bother
test -r /usr/include/SDL/SDL_mixer.h && exit 0

# remove system provided libxml2
# rm -f /usr/lib/libxml2.so.2 /usr/lib/x86_64-linux-gnu/libxml2.so.2 /usr/lib/i386-linux-gnu/libxml2.so.2

# fetch
curl https://www.libsdl.org/projects/SDL_mixer/release/${archive}.tar.gz -o ${archive}.tar.gz || exit $?

# unpack, configure, build, install, cleanup
tar -xzf ${archive}.tar.gz
rm ${archive}.tar.gz
cd ${archive}
CFLAGS=-Os ./configure --prefix=/usr/ || exit $?
make -j 5 || exit $?
make install || exit $?
cd .. && rm -rf ${archive}

