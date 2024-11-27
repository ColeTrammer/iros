FROM ghcr.io/coletrammer/iros_toolchain:iris

ARG USERNAME=ubuntu
ARG USER_UID=1000
ARG USER_GID=$USER_UID
ARG doxygen_version=1.10.0

# Install misc debugging tools
RUN sudo apt-get -y install gdb strace

# Install cmake-format
RUN sudo apt-get -y install pip \
    && pip install cmake-format --break-system-packages

# Install prettier
RUN sudo apt-get -y install npm \
    && sudo npm install -g prettier

# Install just
RUN sudo apt-get -y install just

# Install wayland-dev
RUN sudo apt-get -y install libwayland-dev

# Install jq
RUN sudo apt-get -y install jq

# Install doxygen
RUN apt-get update -y \
    && wget https://www.doxygen.nl/files/doxygen-${doxygen_version}.linux.bin.tar.gz \
    && tar -xzf doxygen-${doxygen_version}.linux.bin.tar.gz \
    && sudo cp doxygen-${doxygen_version}/bin/* --target /usr/local/bin \
    && rm -f doxygen-${doxygen_version}.linux.bin.tar.gz \
    && rm -rf doxygen-${doxygen_version}

# Update core packages
RUN sudo apt-get -y update \
    && sudo apt-get -y upgrade

# Setup non-root user
RUN echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME \
    && mknod /dev/loop0 b 7 0

# Setup persistent command history
RUN SNIPPET="export PROMPT_COMMAND='history -a' && export HISTFILE=/commandhistory/.bash_history" \
    && mkdir /commandhistory \
    && touch /commandhistory/.bash_history \
    && chown -R $USERNAME:$USERNAME /commandhistory \
    && echo "$SNIPPET" >> "/home/$USERNAME/.bashrc" \
    && echo "export PATH=\"\$HOME/.local/bin:\$PATH\"" >> "/home/$USERNAME/.bashrc" \
    && chown $USERNAME:$USERNAME "/home/$USERNAME/.bashrc"
USER ${USERNAME}
