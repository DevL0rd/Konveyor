#!/usr/bin/env bash
set -euo pipefail

if [[ ! -c /dev/udmabuf ]]; then
    sudo modprobe udmabuf
    sudo udevadm settle
fi
sudo chmod a+rw /dev/udmabuf
ls -l /dev/udmabuf
