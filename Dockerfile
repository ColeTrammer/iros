FROM ubuntu:latest as toolchain_build
RUN apt-get update -y && apt-get instcall -y \
    build-essential \
    curl \
    g++-11 \
    gcc-11 \
    git \
    libgmp-dev \
    libmpc-dev \
    libmpfr-dev \
    patchelf \
    tar \
    && rm -rf /var/lib/apt/lists/* \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 900 --slave /usr/bin/g++ g++ /usr/bin/g++-11
ADD / build/iros
WORKDIR /build/iros
RUN FORCE_BUILD_TOOLCHAIN=1 IROS_ARCH=i686 IROS_TOOLCHAIN_PREFIX="/usr/local" ./scripts/setup.sh \
    && FORCE_BUILD_TOOLCHAIN=1 IROS_ARCH=x86_64 IROS_TOOLCHAIN_PREFIX="/usr/local" ./scripts/setup.sh \
    && cd .. && rm -rf iros

FROM ubuntu:latest as toolchain
COPY --from=ghcr.io/coletrammer/iros_toolchain /usr/local /usr/local
RUN apt-get update -y && apt-get install -y \
    ccache \
    clang-format \
    g++-11 \
    gcc-11 \
    genext2fs \
    git \
    grub2 \
    ninja-build \
    nodejs \
    parted \
    patchelf \
    qemu-system-i386 \
    qemu-system-x86 \
    qemu-utils \
    sudo \
    udev \
    xorriso \
    && rm -rf /var/lib/apt/lists/* \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 900 --slave /usr/bin/g++ g++ /usr/bin/g++-11
WORKDIR /build/iros
LABEL org.opencontainers.image.source="https://github.com/ColeTrammer/iros"
