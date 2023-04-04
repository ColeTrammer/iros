FROM ghcr.io/coletrammer/iros_toolchain:iris

ARG USERNAME=iris
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN sudo apt-get -y update \
    && sudo apt-get -y install lsb-release wget software-properties-common gdb gnupg pip strace \
    && wget https://apt.llvm.org/llvm.sh \
    && chmod +x llvm.sh \
    && sudo ./llvm.sh 16 all \
    && pip install cmake-format \
    && rm llvm.sh
RUN sudo apt install -y npm \
    && sudo npm install -g prettier
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID $USERNAME -d /home/$USERNAME -m -k /dev/skel -s /bin/bash \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME \
    && mknod /dev/loop0 b 7 0
RUN SNIPPET="export PROMPT_COMMAND='history -a' && export HISTFILE=/commandhistory/.bash_history" \
    && mkdir /commandhistory \
    && touch /commandhistory/.bash_history \
    && chown -R $USERNAME:$USERNAME /commandhistory \
    && echo "$SNIPPET" >> "/home/$USERNAME/.bashrc" \
    && echo "export PATH=\"\$HOME/.local/bin:\$PATH\"" >> "/home/$USERNAME/.bashrc" \
    && chown $USERNAME:$USERNAME "/home/$USERNAME/.bashrc"
USER iris
