#!/usr/bin/env bash
set -euo pipefail

release=$(uname -r)
if modinfo vgem >/dev/null 2>&1; then
    sudo modprobe vgem
else
    version=$(cut -d. -f1,2 <<<"$release")
    work=$(mktemp -d)
    sudo apt-get update
    sudo apt-get install -y "linux-headers-$release"
    for file in vgem_drv.c vgem_drv.h vgem_fence.c; do
        curl -fsSLo "$work/$file" "https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/plain/drivers/gpu/drm/vgem/$file?h=v$version"
    done
    printf 'obj-m += vgem.o\nvgem-y := vgem_drv.o vgem_fence.o\n' >"$work/Kbuild"
    make -C "/lib/modules/$release/build" M="$work" modules
    sudo insmod "$work/vgem.ko"
fi
sudo chmod a+rw /dev/dri/card* /dev/dri/renderD*
ls -l /dev/dri
