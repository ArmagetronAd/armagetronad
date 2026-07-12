#/usr/bin/env bash

# cleans prerequisites

cd `dirname $0`

yes | conan remove sdl_aa
yes | conan remove aa_libsdl
yes | conan remove libsdl
yes | conan remove sdl

yes | conan remove sdl_image_aa
yes | conan remove aa_libsdl_image
yes | conan remove libsdl_image
yes | conan remove sdl_image

