#!/bin/bash

set -ex
. .travis/docker_container.sh

buildenv bash -c "echo Hello from container" # ensure to start the container
