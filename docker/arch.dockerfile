FROM archlinux:base-20241027.0.273886 AS base
RUN pacman -Sy --noconfirm cmake gcc gcc-libs ninja git pkg-config glibc

FROM base as ci
RUN pacman -S --noconfirm abseil-cpp cxxopts ghc-filesystem jack pugixml simde
