#!/bin/bash

buildenv() {
  setup_container archlinux
  docker exec -w "$(pwd)" -i -t "$container" "$@"
}

setup_container() {
  if [ -f ${TRAVIS_BUILD_DIR}/docker-container-id ]; then
    container=$(cat ${TRAVIS_BUILD_DIR}/docker-container-id)
  else
    container=$(docker run -d -i -t -v /home:/home "$1" /bin/bash)
    echo "$container" > ${TRAVIS_BUILD_DIR}/docker-container-id
  fi
}
