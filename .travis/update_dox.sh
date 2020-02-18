#!/bin/bash

set -x # No fail, we need to go back to the original branch at the end
. .travis/environment.sh

# Build documentation only from Linux x86_64 builds
if [[ ${TRAVIS_CPU_ARCH} != "amd64" || ${TRAVIS_OS_NAME} != "linux" || "${CROSS_COMPILE}" != "" || ${TRAVIS_TAG} == "" ]]; then
  exit 0
fi

doxygen Doxyfile
git fetch --depth=1 https://github.com/${TRAVIS_REPO_SLUG}.git refs/heads/gh-pages:refs/remotes/origin/gh-pages
git checkout origin/gh-pages
git checkout -b gh-pages
mv _api api/${TRAVIS_TAG}
git add api/${TRAVIS_TAG} && git commit -m "Release ${TRAVIS_TAG} (Travis build: ${TRAVIS_BUILD_NUMBER})"
git remote add origin-pages https://${GITHUB_TOKEN}@github.com/${TRAVIS_REPO_SLUG}.git > /dev/null 2>&1
git push --quiet --set-upstream origin-pages gh-pages
git checkout ${TRAVIS_BRANCH}
