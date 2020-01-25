FROM ubuntu:latest

RUN apt-get update && apt-get -y install git g++ curl unzip tar apt-transport-https ca-certificates gnupg software-properties-common wget

RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | apt-key add - \
    && apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main' \
    && apt-get update \
    && apt-get install -y cmake

RUN cd /tmp \
    && git clone https://github.com/Microsoft/vcpkg.git \
    && cd vcpkg \
    && ./bootstrap-vcpkg.sh -disableMetrics \
    && ./vcpkg integrate install

COPY x64-linux-hidden.cmake .

RUN ./tmp/vcpkg/vcpkg --overlay-triplets=. install libsndfile:x64-linux-hidden benchmark:x64-linux-hidden

RUN cd /tmp \
    && git clone --branch develop --recursive https://github.com/sfztools/sfizz.git \
    && cd sfizz \
    && mkdir build \
    && cd build \
    && cmake .. -DSFIZZ_JACK=OFF -DSFIZZ_SHARED=OFF -DSFIZZ_LV2=ON "-DCMAKE_TOOLCHAIN_FILE=/tmp/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-linux-hidden -DCMAKE_BUILD_TYPE=Release -DSFIZZ_USE_VCPKG=ON\
    && make -j 16
