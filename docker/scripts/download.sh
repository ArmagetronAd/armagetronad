#!/bin/bash

# Usage: download.sh <local_file> <URI>
# Downloads URI to local_file, keeping a cache based on the base name

download_dir="`dirname $0`/.cache"
mkdir -p ${download_dir} || exit 1
mkdir -p "`dirname $1`" || exit 1
fn="`basename $1`"

test -f "${download_dir}/${fn}" || wget "$2" -O "${download_dir}/${fn}" || exit 1
test -f "$1" || ln -f "${download_dir}/${fn}" "$1"  || exit 1

