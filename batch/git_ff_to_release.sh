#!/bin/sh

# Depending on which branch this is invoked on, fast forward from next a little less stable branch
# master -> beta, beta -> release

#set -x

sd=$1
test -z "${sd}" && sd=`dirname $0`

HASH=`git -C ${sd} rev-parse HEAD`
BRANCH=`git -C ${sd} rev-parse --abbrev-ref HEAD`
# BRANCH=beta_0.2.9
SUFFIX=`echo ${BRANCH} | sed -e s/^release// -e s/^beta// -e s/^legacy//`

MERGE_FROM=""

case ${BRANCH} in
    release*)
        MERGE_FROM=beta${SUFFIX}
        ;;
    beta)
        MERGE_FROM=master
        ;;
    beta*)
        if test "${SUFFIX}" = "_0.2.9"; then
            SUFFIX=_0.2.8
        fi
        MERGE_FROM=legacy${SUFFIX}
        ;;
    *)
        echo "Unknown branch, don't know what to do."
        exit 0
        ;;
esac

git -C ${sd} fetch origin || exit $?

# merge the listed branches from origin and, if present, locally
TOTAL_MERGE=""
for s in ${MERGE_FROM}; do
    TOTAL_MERGE="${TOTAL_MERGE} origin/$s"
    if git -C ${sd} branch -l | grep -q " $s\$"; then
        TOTAL_MERGE="${TOTAL_MERGE} ${s}"
    fi
done

git -C ${sd} merge --ff-only ${TOTAL_MERGE} || exit $?

NEW_HASH=`git -C ${sd} rev-parse HEAD`
test ${NEW_HASH} = ${HASH} && exit 0

echo
echo '****************************'
echo 'New revisions in this merge:'
echo '****************************'
git log ${HASH}..

echo
echo In case you change your mind and 'git reset --hard', the pre-merge git revision is:
echo ${HASH}