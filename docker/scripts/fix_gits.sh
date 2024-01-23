#!/bin/bash
wd=`dirname $0`
download_dir="${wd}/.cache/gits"

function fix_git(){
    name=$1
    rev=$2    

    dir=${download_dir}/${name}
    pushd ${dir} || return $?
    RETURN=0
    if ! git reset ${rev} --hard; then
        git fetch
        git reset ${rev} --hard || RETURN=$?
    fi
    popd
    return ${RETURN}
}

function fix_gits(){
    ${wd}/ensure_gits.sh hack-0.2.8-sty+ct+ap || return $?

    set -x
    # On merge conflicts here: run update_gits.sh, that sorts things out

    fix_git winlibs d03e20bf8973a6cbd1e0ecb47dd8333c5614b139 || return $?
    fix_git codeblocks 2f363cec627bc40bdc358c2cc467d3384910691c || return $?
    fix_git steam-art 7b3930ab85a3dfbb1caffaa17442b670c823f314 || return $?
    fix_git pkg2appimage 678e5e14122f14a12c54847213585ea803e1f0e1 || return $?
}

if ! fix_gits; then
    # clean and retry on error
    rm -rf ${download_dir}
    fix_gits || exit $?
fi
