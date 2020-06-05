#!/bin/sh

# Depending on which branch this is invoked on, merge all other branches that can be deemed safe

#set -x

sd=$1
test -z "${sd}" && sd=`dirname $0`

HASH=`git -C ${sd} rev-parse HEAD`
BRANCH=`git -C ${sd} rev-parse --abbrev-ref HEAD`
#BRANCH=beta_0.2.9
SUFFIX=`echo ${BRANCH} | sed -e s/^release// -e s/^beta// -e s/^legacy//`

MERGE_FROM=""

case ${BRANCH} in
    release*)
        echo "Release branch, nothing safe to merge."
        exit 0
        ;;
    beta*)
        MERGE_FROM=release${SUFFIX}
        ;;
    legacy*)
        if test "${SUFFIX}" = "_0.2.8"; then
            SUFFIX=_0.2.9
            MERGE_FROM=legacy_0.2.8.3
        fi
        MERGE_FROM="${MERGE_FROM} release${SUFFIX} beta${SUFFIX}"
        ;;
    master)
        MERGE_FROM="legacy_0.2.8 legacy_0.2.8.3"
        #MERGE_FROM="release beta legacy_0.2.8 legacy_0.2.8.3"
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

git -C ${sd} merge ${TOTAL_MERGE} || exit $?

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