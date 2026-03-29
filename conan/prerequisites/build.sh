#/usr/bin/env bash

# builds prerequisites and puts them into your local conan repository

cd `dirname $0`

yes | conan remove sdl_aa
yes | conan remove aa_libsdl
yes | conan remove libsdl
yes | conan remove sdl
conan create sdl

yes | conan remove sdl_image_aa
yes | conan remove aa_libsdl_image
yes | conan remove libsdl_image
yes | conan remove sdl_image
conan install sdl_image --build=missing
conan create sdl_image

