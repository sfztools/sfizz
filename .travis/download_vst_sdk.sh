#!/bin/bash
set -ex

vst_download_prefix="vst/download"
vst_sdk_archive="vst-sdk_3.6.14_build-24_2019-11-29.zip"
mkdir -p ${vst_download_prefix}
if ! [[ -f "${vst_download_prefix}/${vst_sdk_archive}" ]]; then
  wget -P ${vst_download_prefix} "https://download.steinberg.net/sdk_downloads/${vst_sdk_archive}"
fi
mkdir -p vst/external
unzip -ouq "${vst_download_prefix}/${vst_sdk_archive}" -d "vst/external"
