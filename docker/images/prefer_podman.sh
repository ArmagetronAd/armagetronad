# prefer podman over docker, works rootless
if podman -h > /dev/null; then
    function docker(){
        podman $*
    }
fi

