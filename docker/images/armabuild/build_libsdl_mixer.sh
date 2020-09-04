#!/bin/bash

# usage: $0 <name of library directory>, e.g. SDL_mixer-1.2.12
set -x
archive=$1

# if it already exists (Steam SDK), don't bother
test -r /usr/include/SDL2/SDL_mixer.h && exit 0

# remove system provided libxml2
# rm -f /usr/lib/libxml2.so.2 /usr/lib/x86_64-linux-gnu/libxml2.so.2 /usr/lib/i386-linux-gnu/libxml2.so.2

#flags stolen from debian
confflags_debian="\
--enable-music-cmd \
--disable-music-mp3-mad-gpl \
--enable-music-mod \
--enable-music-mod-modplug \
--disable-music-mod-mikmod \
--enable-music-ogg \
--enable-music-wave \
--disable-music-flac-shared \
--disable-music-ogg-shared \
--disable-music-mod-mikmod-shared \
--disable-music-mod-modplug-shared \
"

# not taken over
#--disable-music-fluidsynth-shared
#--enable-music-mp3 \
#--enable-music-midi \
#--enable-music-midi-fluidsynth \
#--enable-music-midi-timidity \

# our modifications: Disable formats we do not want
confflags_aa="\
--disable-music-mp3 \
--disable-music-flac \
--disable-music-midi \
"

# fetch
curl https://www.libsdl.org/projects/SDL_mixer/release/${archive}.tar.gz -o ${archive}.tar.gz || exit $?

# unpack, configure, build, install, cleanup
tar -xzf ${archive}.tar.gz
rm ${archive}.tar.gz
cd ${archive}

CFLAGS=-Os ./configure --prefix=/usr/ ${confflags_debian} ${confflags_aa} || exit $?
make -j 5 || exit $?
make install || exit $?
cd .. && rm -rf ${archive}

