#!/bin/bash

# usage: $0 <name of library directory>, e.g. SDL_image-1.2.12
set -x
archive=$1

# if it already exists (Steam SDK), don't bother
test -r /usr/include/SDL/SDL_image.h && exit 0

# from debian, mostly
confflags="--disable-jpg-shared \
--disable-tif \
--disable-png-shared \
"

# fetch
curl https://www.libsdl.org/projects/SDL_image/release/${archive}.tar.gz -o ${archive}.tar.gz || exit $?

# unpack, configure, build, install, cleanup
tar -xzf ${archive}.tar.gz
rm ${archive}.tar.gz
cd ${archive}
CFLAGS="-Os -pipe" LDFLAGS="-Wl,--as-needed" ./configure --prefix=/usr/ ${confflags} || exit $?
make -j 5 || exit $?
make install || exit $?
cd .. && rm -rf ${archive}

