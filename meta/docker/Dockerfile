FROM ubuntu:latest as toolchain_build
RUN apt-get update -y \
    && apt-get install -y gpg software-properties-common wget \
    && wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null \
    && apt-add-repository -y 'deb https://apt.kitware.com/ubuntu/ jammy main' \
    && apt-get purge -y --auto-remove gpg software-properties-common wget \
    && apt-get update -y \
    && apt-get install -y cmake \
    && rm -rf /var/lib/apt/lists/*
RUN apt-get update -y && apt-get install -y \
    bison \
    build-essential \
    ccache \
    flex \
    g++-12 \
    gcc-12 \
    git \
    libgmp-dev \
    libisl-dev \
    libmpc-dev \
    libmpfr-dev \
    ninja-build \
    texinfo \
    && rm -rf /var/lib/apt/lists/* \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 900 --slave /usr/bin/g++ g++ /usr/bin/g++-12
ADD / build/iros
WORKDIR /build/iros
RUN IROS_PREFIX="/usr/local" ./meta/toolchain/build.sh \
    && cd .. && rm -rf iros

FROM ubuntu:latest as toolchain
RUN apt-get update -y \
    && apt-get install -y gpg software-properties-common wget \
    && wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null \
    && apt-add-repository -y 'deb https://apt.kitware.com/ubuntu/ jammy main' \
    && apt-get purge -y --auto-remove gpg software-properties-common wget \
    && apt-get update -y \
    && apt-get install -y cmake \
    && rm -rf /var/lib/apt/lists/*
RUN apt-get update -y && apt-get install -y \
    build-essential \
    ccache \
    dosfstools \
    g++-12 \
    gcc-12 \
    git \
    ninja-build \
    parted \
    qemu-system-x86 \
    qemu-utils \
    sudo \
    udev \
    && rm -rf /var/lib/apt/lists/* \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 900 --slave /usr/bin/g++ g++ /usr/bin/g++-12
COPY --from=toolchain_build /usr/local /usr/local
WORKDIR /build/iros
LABEL org.opencontainers.image.source="https://github.com/ColeTrammer/iros"
