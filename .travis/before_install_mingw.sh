#!/bin/bash

set -ex
. .travis/docker_container.sh

buildenv bash -c "echo Hello from container" # ensure to start the container
docker cp "$container":/etc/pacman.conf pacman.conf
cat >>pacman.conf <<EOF
[multilib]
Include = /etc/pacman.d/mirrorlist

[mingw-w64]
SigLevel = Optional TrustAll
Server = https://github.com/jpcima/arch-mingw-w64/releases/download/repo.\$arch/
EOF
docker cp pacman.conf "$container":/etc/pacman.conf
rm -f pacman.conf
