#!/bin/bash
# Must be called from the root directory
if [[ -f api_index.md ]]; then rm api_index.md; fi
cat >>api_index.md <<EOF
---
title: "API"
---
EOF
tags=()
for tag in $(git tag --list); do
  if [[ ${tag} != "list" && ${tag} != *"test"* ]]; then
    tag=$(echo "${tag}" | sed -r 's/v//g')
    tags+=("${tag}")
  fi
done
IFS=$'\n' tags=($(sort <<<"${tags[*]}")); unset IFS
for tag in "${tags[@]}"; do
  echo "- [${tag}](${tag})" >> api_index.md
done
