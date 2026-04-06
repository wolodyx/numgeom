FROM ubuntu:24.04

ENV TZ=Europe/Moscow
RUN    ln -snf /usr/share/zoneinfo/$TZ /etc/localtime \
    && echo $TZ > /etc/timezone

RUN apt update               \
 && apt -y install           \
        git                  \
        cmake                \
        build-essential      \
        ninja-build          \
        flex                 \
        bison                \
        python3              \
        wget                 \
        curl

RUN wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc | tee /etc/apt/trusted.gpg.d/lunarg.asc \
 && wget -qO /etc/apt/sources.list.d/lunarg-vulkan-noble.list http://packages.lunarg.com/vulkan/lunarg-vulkan-noble.list

RUN apt update               \
 && apt -y install           \
      vulkan-sdk             \
      libboost-log-dev       \
      qtbase5-dev            \
      qtbase5-dev-tools      \
      libocct-data-exchange-dev \
      libocct-draw-dev       \
      libocct-foundation-dev \
      libocct-modeling-algorithms-dev \
      libocct-modeling-data-dev \
      libocct-ocaf-dev       \
      libocct-visualization-dev \
      libxrandr-dev          \
      libxinerama-dev        \
      libxcursor-dev         \
      libxi-dev              \
      libtbb-dev             \
      wayland-protocols      \
      libwayland-dev         \
      libwayland-client0

# Install linuxdeploy to generate the appimage packages.
COPY install-linuxdeploy.sh /tmp/install-linuxdeploy.sh
RUN  /tmp/install-linuxdeploy.sh

WORKDIR /usr/local/src

ARG USERNAME=tim
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN    userdel -r ubuntu || true \
    && groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

USER $USERNAME
