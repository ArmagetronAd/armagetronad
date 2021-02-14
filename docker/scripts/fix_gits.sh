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
    ${wd}/ensure_gits.sh legacy_0.2.9 || return $?

    set -x
    # On merge conflicts here: run update_gits.sh, that sorts things out

    fix_git macOS c77e2d9a24e48f36f1ddab7c76f140e7f145ccf1 || return $?
    fix_git winlibs d03e20bf8973a6cbd1e0ecb47dd8333c5614b139 || return $?
    fix_git codeblocks c633fa6599c888d92156648a882d76f3ca15b610 || return $?
    fix_git steam-art 7b3930ab85a3dfbb1caffaa17442b670c823f314 || return $?
    fix_git pkg2appimage 678e5e14122f14a12c54847213585ea803e1f0e1 || return $?
}

if ! fix_gits; then
    # clean and retry on error
    rm -rf ${download_dir}
    fix_gits || exit $?
fi
