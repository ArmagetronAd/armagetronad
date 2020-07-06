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
    ${wd}/ensure_gits.sh master || return $?

    set -x

    fix_git winlibs dd07262970713dcf0e193d0608178963af73eb15 || return $?
    fix_git steam-art 42688f1bcf1bf1f2befc81a1143fb20f346b92f8 || return $?
}

if ! fix_gits; then
    # clean and retry on error
    rm -rf ${download_dir}
    fix_gits || exit $?
fi
