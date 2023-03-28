FROM ghcr.io/coletrammer/iros_toolchain:iris

ARG USERNAME=iris
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN sudo apt-get -y update \
    && sudo apt-get -y install lsb-release wget software-properties-common gdb gnupg strace \
    && wget https://apt.llvm.org/llvm.sh \
    && chmod +x llvm.sh \
    && sudo ./llvm.sh 16 \
    && rm llvm.sh
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID $USERNAME -d /home/$USERNAME -m -k /dev/skel -s /bin/bash \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME \
    && mknod /dev/loop0 b 7 0
USER iris
