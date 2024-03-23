FROM iros_toolchain:latest

ARG USERNAME=iris
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID $USERNAME -d /home/$USERNAME -m -k /dev/skel -s /bin/bash \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && echo 'export PATH="/build/iros/build_docker/userland/edit:$PATH"' >/home/iris/.bashrc \
    && echo 'export PATH="/build/iros/build_docker/gen/port_manager:$PATH"' >>/home/iris/.bashrc \
    && echo 'alias pm=port_manager' >>/home/iris/.bashrc \
    && chmod 0440 /etc/sudoers.d/$USERNAME
USER iris
