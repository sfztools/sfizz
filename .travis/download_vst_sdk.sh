#!/bin/bash
set -ex

vst_sdk_archive="vst-sdk_3.6.14_build-24_2019-11-29.zip"
wget "https://download.steinberg.net/sdk_downloads/${vst_sdk_archive}"
mkdir -p vst/external
unzip -ouq ${vst_sdk_archive} -d vst/external
