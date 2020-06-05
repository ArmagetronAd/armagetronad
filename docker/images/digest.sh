# sets REFERENCE to the best reference to docker image $1,
# either :${EPOCH} for local builds or @sha256:.... for already uploaded ones

# set -x

id=`dirname ${0}`/../images
. ${id}/epoch.sh || exit $?
if test -r ${id}/$1.digest.local || ! test -r ${id}/$1.digest; then
    REFERENCE=:${EPOCH}
else
    REFERENCE=@`cat ${id}/$1.digest`
fi
