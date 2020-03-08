#!/bin/bash
# Must be called from the root directory
cat >>index.md <<EOF
---
title: "API"
---
EOF
for tag in $(git tag --list); do
  if [[ ${tag} != "list" && ${tag} != *"test"* ]]; then
    echo "- [${tag}](${tag})" >> index.md
  fi
done
