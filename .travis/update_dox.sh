#!/bin/bash

set -x # No fail, we need to go back to the original branch at the end
. .travis/environment.sh

mkdir build && cd build && cmake -DSFIZZ_JACK=OFF -DSFIZZ_SHARED=OFF -DSFIZZ_LV2=OFF .. && cd ..
doxygen Doxyfile
./doxygen/scripts/generate_api_index.sh
git fetch --depth=1 https://github.com/${TRAVIS_REPO_SLUG}.git refs/heads/gh-pages:refs/remotes/origin/gh-pages
git checkout origin/gh-pages
git checkout -b gh-pages
mv _api api/${TRAVIS_TAG}
mv api_index.md api/index.md
git add api && git commit -m "Release ${TRAVIS_TAG} (Travis build: ${TRAVIS_BUILD_NUMBER})"
git remote add origin-pages https://${GITHUB_TOKEN}@github.com/${TRAVIS_REPO_SLUG}.git > /dev/null 2>&1
git push --quiet --set-upstream origin-pages gh-pages
git checkout ${TRAVIS_BRANCH}
