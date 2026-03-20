FROM ubuntu:24.04

ENV TZ=Europe/Moscow
RUN    ln -snf /usr/share/zoneinfo/$TZ /etc/localtime \
    && echo $TZ > /etc/timezone

RUN apt update &&            \
    apt -y install           \
        git                  \
        cmake                \
        build-essential      \
        ninja-build          \
        flex                 \
        bison                \
        curl                 \
        zip                  \
        unzip                \
        tar                  \
        file                 \
        pkg-config           \
        autoconf             \
        autoconf-archive     \
        automake             \
        libtool              \
        python3-venv         \
        python3-pip          \
        wayland-protocols    \
        libwayland-dev       \
        libwayland-client0   \
        libxtst-dev          \
        libxinerama-dev      \
        libxcursor-dev       \
        '^libxcb.*-dev'      \
        libx11-xcb-dev       \
        libgl1-mesa-dev      \
        libxrender-dev       \
        libxi-dev            \
        libxkbcommon-dev     \
        libxkbcommon-x11-dev \
        libsystemd-dev       \
        libarchive-dev       \
        libxrandr-dev

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
