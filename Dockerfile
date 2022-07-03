FROM ubuntu:latest as toolchain_build
RUN apt-get update -y && apt-get install -y \
    build-essential \
    cmake \
    curl \
    g++-12 \
    gcc-12 \
    git \
    libgmp-dev \
    libisl-dev \
    libmpc-dev \
    libmpfr-dev \
    libssl-dev \
    tar \
    texinfo \
    && rm -rf /var/lib/apt/lists/* \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 900 --slave /usr/bin/g++ g++ /usr/bin/g++-12
ADD / build/iros
WORKDIR /build/iros
RUN FORCE_BUILD_TOOLCHAIN=1 IROS_ARCH=i686 IROS_TOOLCHAIN_PREFIX="/usr/local" ./scripts/setup.sh \
    && FORCE_BUILD_TOOLCHAIN=1 IROS_ARCH=x86_64 IROS_TOOLCHAIN_PREFIX="/usr/local" ./scripts/setup.sh \
    && cd .. && rm -rf iros

FROM ubuntu:latest as toolchain
RUN apt-get update -y && apt-get install -y \
    build-essential \
    ccache \
    clang-format \
    curl \
    g++-12 \
    gcc-12 \
    genext2fs \
    git \
    grub2 \
    libsdl2-dev \
    ninja-build \
    nodejs \
    parted \
    qemu-system-i386 \
    qemu-system-x86 \
    qemu-utils \
    sudo \
    udev \
    xorriso \
    valgrind \
    && rm -rf /var/lib/apt/lists/* \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 900 --slave /usr/bin/g++ g++ /usr/bin/g++-12
COPY --from=toolchain_build /usr/local /usr/local
WORKDIR /build/iros
LABEL org.opencontainers.image.source="https://github.com/ColeTrammer/iros"
