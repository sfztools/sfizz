# ---------------------------------------------------------------------------- #
# Global environment included from all scripts                                 #
# ---------------------------------------------------------------------------- #

# add /usr/local/bin to path, for custom cmake installs
if [[ ${TRAVIS_OS_NAME} == "linux" || ${TRAVIS_OS_NAME} == "osx" ]]; then
  export PATH="/usr/local/bin:$PATH"
fi

# buildenv runs a command in host machine or container, depending on build
if [[ ${CROSS_COMPILE} == "mingw32" ]]; then
  buildenv() {
    setup_container archlinux
    docker exec -w "$(pwd)" -i -t "$container" "$@"
  }
else
  buildenv() {
    "$@"
  }
fi

# create a container based on given image if not existing
# the container id is persisted in a file across different scripts
setup_container() {
    if [ -f ${TRAVIS_BUILD_DIR}/docker-container-id ]; then
      container=$(cat ${TRAVIS_BUILD_DIR}/docker-container-id)
    else
      container=$(docker run -d -i -t -v /home:/home "$1" /bin/bash)
      echo "$container" > ${TRAVIS_BUILD_DIR}/docker-container-id
    fi
}
