#!/bin/bash

if [ -z "$CONTAINER" ]; then
  echo "The variable CONTAINER is not set."
  exit 1
fi

buildenv() {
  setup_container "$CONTAINER"
  docker exec -w "$(pwd)" -u "$(id -u)" -i -t "$container" "$@"
}

buildenv_as_root() {
  setup_container "$CONTAINER"
  docker exec -w "$(pwd)" -u 0 -i -t "$container" "$@"
}

setup_container() {
  if [ -f ${TRAVIS_BUILD_DIR}/docker-container-id ]; then
    container=$(cat ${TRAVIS_BUILD_DIR}/docker-container-id)
  else
    container=$(docker run -d -i -t -v /home/travis:/home/travis "$1" /bin/bash)
    echo "$container" > ${TRAVIS_BUILD_DIR}/docker-container-id
  fi
}
