FROM ghcr.io/coletrammer/iros_toolchain:iris

ARG USERNAME=iris
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# Install clang
RUN sudo apt-get -y update \
    && sudo apt-get -y install lsb-release wget software-properties-common gnupg \
    && wget https://apt.llvm.org/llvm.sh \
    && chmod +x llvm.sh \
    && sudo ./llvm.sh 16 all \
    && rm llvm.sh

# Install dotnet runtime
RUN sudo apt-get -y install aspnetcore-runtime-6.0

# Install misc debugging tools
RUN sudo apt-get -y install gdb strace

# Install cmake-format
RUN sudo apt-get -y install pip \
    && pip install cmake-format

# Install prettier
RUN sudo apt-get -y install npm \
    && sudo npm install -g prettier

# Update core packages
RUN sudo apt-get -y update \
    && sudo apt-get -y upgrade

# Setup non-root user
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID $USERNAME -d /home/$USERNAME -m -k /dev/skel -s /bin/bash \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
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
USER iris
