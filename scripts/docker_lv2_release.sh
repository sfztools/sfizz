#!/bin/bash
script_dir="$(dirname "$0")"
pushd $script_dir
docker build -t sfizz .
popd
docker cp $(docker create sfizz:latest):/tmp/sfizz/build/sfizz.lv2 .