#!/bin/bash
wd=`dirname $0`
download_dir="${wd}/.cache/gits"

function fix_git(){
    name=$1
    rev=$2    

    dir=${download_dir}/${name}
    if ! git -C ${dir} reset ${rev} --hard; then
        git -C ${dir} fetch
	git -C ${dir} reset ${rev} --hard || return $?
    fi
}

function fix_gits(){
    ${wd}/ensure_gits.sh legacy_0.2.8.3 || return $?

    set -x

    fix_git macOS c77e2d9a24e48f36f1ddab7c76f140e7f145ccf1 || return $?
    fix_git winlibs d03e20bf8973a6cbd1e0ecb47dd8333c5614b139 || return $?
    fix_git codeblocks 5ab4c626eb7cfa692a0bda7dbf26fed3721e4aa3 || return $?
    fix_git ubuntu c4910e573fefd62f8241d4a484482ab8bb205b4d || return $?
}

if ! fix_gits; then
    # clean and retry on error
    rm -rf ${download_dir}
    fix_gits || exit $?
fi
