#!/bin/bash

# usage: build_libcurl.sh

set -x
archive=curl-8.12.1

# if it already exists, don't bother
test -r /usr/lib/libcurl.* && exit 0
test -r /usr/lib/x86_64-linux-gnu/libcurl.* && exit 0

# fetch
curl https://curl.se/download/${archive}.tar.bz2 -o ${archive}.tar.bz2 || exit $?

# unpack, configure, build, install, cleanup
tar -xjf ${archive}.tar.bz2
rm ${archive}.tar.bz2
cd ${archive}

# ./configure --help; exit 1

# the previous fetch library, nanoHTTP, did not support https,
# so in order to not create accidental incompatibilities, we also
# do not support https here. But we still need the curl program
# to work; we just have the curl built here renamed.

CFLAGS=-Os ./configure --prefix=/usr/ --program-prefix=nossl- \
	--enable-http --enable-ftp \
	--disable-https \
	--without-ssl \
	--disable-file \
	--disable-ipfs \
	--disable-ldap \
	--disable-dpaps \
	--disable-rtsp \
	--disable-dict \
	--disable-telnet \
	--disable-tftp \
	--disable-pop3 \
	--disable-imap \
	--disable-smb \
	--disable-smtp \
	--disable-gopher \
	--disable-qmtt \
	--disable-manual \
	--disable-docs \
	--without-libpsl \
	--disable-curl \
	|| exit $?

	# --with-openssl \

make -j 5 || exit $?
make install || exit $?
cd .. && rm -rf ${archive}

