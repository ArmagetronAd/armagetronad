#!/usr/bin/env bash

echo "Begin conan $*"

set -x
conan $* \
	--build=missing \
    -o:a="libxml2/*:html=False" \
    -o:a="libxml2/*:http=False" \
    -o:a="libxml2/*:ftp=False" \
    -o:a="libxml2/*:zlib=False" \
    -o:a="libxml2/*:iconv=False" \
    -o:a="sdl/*:directx=False" \
    -o:a="sdl/*:x11=False" \
    -o:a="sdl/*:xvm=False" \
    -o:a="sdl/*:xcb=False" \
    -o:a="sdl/*:vulkan=False" \
    -o:a="sdl/*:opengles=False" \
    -o:a="sdl/*:libunwind=False" \
    -o:a="sdl/*:hidapi=False" \
    -o:a="sdl/*:xshape=False" \
    -o:a="sdl/*:xcursor=False" \
    -o:a="sdl/*:xinput=False" \
    -o:a="sdl/*:xrandr=False" \
    -o:a="sdl/*:xinerama=False" \
    -o:a="libcurl/*:with_ssl=False" \
    -o:a="libcurl/*:with_https=False" \
    -o:a="libcurl/*:with_ftp=False" \
    -o:a="libcurl/*:with_file=False" \
    -o:a="libcurl/*:with_rtsp=False" \
    -o:a="libcurl/*:with_dict=False" \
    -o:a="libcurl/*:with_telnet=False" \
    -o:a="libcurl/*:with_tftp=False" \
    -o:a="libcurl/*:with_pop3=False" \
    -o:a="libcurl/*:with_imap=False" \
    -o:a="libcurl/*:with_smtp=False" \
    -o:a="libcurl/*:with_gopher=False" \
    -o:a="libcurl/*:static=False" \
	-o:a="*/*:flac=False" \
	-o:a="*/*:shared=True" -v \
	|| exit $?

echo "End conan $*"

