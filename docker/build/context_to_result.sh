#!/bin/bash

# runs a context through dockerbuild
# usage: context_to_result.sh <image> <name> <command>
# takes input: context.<name>
# produces output: result.<name>
# name can also carry a context. or result. prefix, it will be removed

#echo -e "\e]2;Build $1\a"

set -x

wd="`dirname $0`"
bic="`readlink -f ${wd}/build_in_container.sh`"

name="$1"
shift

if echo ${name} | grep "^context\." > /dev/null; then
    name=`echo ${name} | sed -e s,context\.,,`
fi
if echo ${name} | grep "^result\." > /dev/null; then
    name=`echo ${name} | sed -e s,result\.,,`
fi

rm -rf result.${name}
cd context.${name} || exit $?
image=`cat image` || exit $?

#sleep 1

find . -name "*~" -exec rm \{\} \;

set +x
echo -e "\n\n********************************************************************"
echo -e "Build ${name}"
echo -e "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n\n"

${bic} "${image}" ../result.${name} "$@" || exit $?

echo -e "\n\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
echo -e "Done building ${name}"
echo -e "********************************************************************"
