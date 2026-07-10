#/usr/bin/env bash

# builds prerequisites and puts them into your local conan repository

cd `dirname $0`

conan install sdl --build=missing
conan create sdl --build=missing

conan install sdl_image --build=missing
conan create sdl_image --build=missing

