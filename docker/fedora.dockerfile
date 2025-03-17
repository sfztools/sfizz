FROM fedora:41 AS base
RUN dnf install -y cmake g++ git pkg-config ninja-build

FROM base AS ci
RUN dnf install -y abseil-cpp-devel simde-devel
