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

    fix_git winlibs d03e20bf8973a6cbd1e0ecb47dd8333c5614b139 || return $?
    fix_git codeblocks 4a51998b7f12723c40cca3e2558da3ab8af85814 || return $?
    fix_git steam-art 42688f1bcf1bf1f2befc81a1143fb20f346b92f8 || return $?
}

if ! fix_gits; then
    # clean and retry on error
    rm -rf ${download_dir}
    fix_gits || exit $?
fi
