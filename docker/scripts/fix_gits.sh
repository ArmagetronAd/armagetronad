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
    ${wd}/ensure_gits.sh trunk || return $?

    set -x
    # On merge conflicts here: run update_gits.sh, that sorts things out

    fix_git winlibs 1ed9319621779ddb76837ce645e1a68ba7f131bd || return $?
    fix_git steam-art 42688f1bcf1bf1f2befc81a1143fb20f346b92f8 || return $?
    fix_git pkg2appimage 678e5e14122f14a12c54847213585ea803e1f0e1 || return $?
}

if ! fix_gits; then
    # clean and retry on error
    rm -rf ${download_dir}
    fix_gits || exit $?
fi
