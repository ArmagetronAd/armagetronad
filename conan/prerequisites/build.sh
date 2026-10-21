#/usr/bin/env bash

# builds prerequisites and puts them into your local conan repository

cd `dirname $0`

conan install sdl --build=missing || exit $?
conan create sdl --build=missing || exit $?

conan install sdl_image --build=missing || exit $?
conan create sdl_image --build=missing || exit $?

