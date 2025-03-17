FROM ubuntu:24.04 AS base
RUN apt update -y
RUN apt install -y cmake g++ git pkg-config ninja-build

FROM base as ci
RUN apt install libabsl-dev

