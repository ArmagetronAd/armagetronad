# prefer podman over docker, works rootless
if podman -h 2>&1 > /dev/null; then
    function docker(){
        podman $*
    }
fi

