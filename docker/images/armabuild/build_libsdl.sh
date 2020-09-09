#!/bin/bash

# usage: $0 <name of library directory>, e.g. SDL-1.2.15

set -x
archive=$1

# if it already exists (Steam SDK), don't bother
test -d /usr/include/SDL && exit 0

# from debian
confflags="--disable-rpath --enable-sdl-dlopen \
           --disable-video-ggi --disable-video-svga --disable-video-aalib \
           --disable-nas --disable-esd --disable-arts \
           "

# not taken from debian
#           --enable-video-caca \
# --disable-pulseaudio-shared \
#           --disable-alsa-shared \
#           --disable-x11-shared \

# ours
confflags_ours="--disable-assembly \
--disable-video-fbcon --disable-video-directfb \
--disable-x11-dgamouse --disable-video-dga --disable-dga \
 --disable-alsa-test --enable-alsa --disable-arts --disable-esd --enable-audio \
 --enable-pulseaudio-shared --enable-alsa-shared --enable-x11-shared"

#--disable-alsa --enable-pulseaudio

# fetch
curl https://www.libsdl.org/release/${archive}.tar.gz -o ${archive}.tar.gz || exit $?

# unpack, configure, build, install, cleanup
tar -xzf ${archive}.tar.gz
rm ${archive}.tar.gz
cd ${archive}
patch -Np1 -i ../libsdl-1.2.15-const-xdata32.patch || exit $?
rm -f ../libsdl-1.2.15-const-xdata32.patch
CFLAGS=-Os ./configure --prefix=/usr/ ${confflags_ours} ${confflags} || exit $?
make -j 5 || exit $?
make install || exit $?
cd .. && rm -rf ${archive}

