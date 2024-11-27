FROM ubuntu:24.04 AS toolchain_build
ARG GCC_VERSION=14
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
    curl \
    flex \
    g++-$GCC_VERSION \
    gcc-$GCC_VERSION \
    git \
    libgmp-dev \
    libisl-dev \
    libmpc-dev \
    libmpfr-dev \
    autoconf2.69 \
    ninja-build \
    texinfo \
    && rm -rf /var/lib/apt/lists/* \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-$GCC_VERSION 900 --slave /usr/bin/g++ g++ /usr/bin/g++-$GCC_VERSION
COPY / build/iros
WORKDIR /build/iros
RUN IROS_PREFIX="/usr/local" ./meta/toolchain/build.sh \
    && cd .. && rm -rf iros

FROM ubuntu:24.04 AS toolchain

# Latest CMake
RUN apt-get update -y \
    && apt-get install -y gpg software-properties-common wget \
    && wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null \
    && apt-add-repository -y 'deb https://apt.kitware.com/ubuntu/ jammy main' \
    && apt-get purge -y --auto-remove gpg software-properties-common wget \
    && apt-get update -y \
    && apt-get install -y cmake \
    && rm -rf /var/lib/apt/lists/*

# Install clang
ARG LLVM_VERSION=19
RUN apt-get -y update \
    && apt-get -y install lsb-release wget software-properties-common gnupg \
    && wget https://apt.llvm.org/llvm.sh \
    && chmod +x llvm.sh \
    && ./llvm.sh ${LLVM_VERSION} all \
    && rm llvm.sh \
    && apt-get -y install clang-${LLVM_VERSION} clang-tools-${LLVM_VERSION} clang-format-${LLVM_VERSION} clang-tidy-${LLVM_VERSION} clangd-${LLVM_VERSION} \
    && update-alternatives \
    --install /usr/bin/clang clang /usr/bin/clang-${LLVM_VERSION} ${LLVM_VERSION} \
    --slave /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION} \
    --slave /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-${LLVM_VERSION}  \
    --slave /usr/bin/clang-format clang-format /usr/bin/clang-format-${LLVM_VERSION} \
    --slave /usr/bin/clangd clangd /usr/bin/clangd-${LLVM_VERSION} \
    --slave /usr/bin/run-clang-tidy run-clang-tidy /usr/bin/run-clang-tidy-${LLVM_VERSION} \
    && rm -rf /var/lib/apt/lists/*

# Misc packages + GCC
ARG GCC_VERSION=14
RUN apt-get update -y && apt-get install -y \
    build-essential \
    ccache \
    dosfstools \
    g++-$GCC_VERSION \
    gcc-$GCC_VERSION \
    git \
    graphviz \
    ninja-build \
    parted \
    qemu-system-x86 \
    qemu-utils \
    sudo \
    udev \
    && rm -rf /var/lib/apt/lists/* \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-$GCC_VERSION 900 --slave /usr/bin/g++ g++ /usr/bin/g++-$GCC_VERSION  --slave /usr/bin/gcov gcov /usr/bin/gcov-$GCC_VERSION

# Custom toolchain
COPY --from=toolchain_build /usr/local /usr/local
WORKDIR /build/iros
LABEL org.opencontainers.image.source="https://github.com/ColeTrammer/iros"
