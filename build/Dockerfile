FROM ubuntu:22.04

ENV TZ=Europe/Moscow
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN  apt-get update     \
  && apt-get -y install \
        build-essential \
        git             \
        cmake           \
        flex            \
        bison           \
        curl            \
        zip             \
        unzip           \
        tar             \
        file            \
        pkg-config

WORKDIR /usr/local/src

# Install vcpkg
RUN   git clone https://github.com/microsoft/vcpkg.git \
   && cd vcpkg \
   && ./bootstrap-vcpkg.sh -disableMetrics \
   && mv vcpkg /usr/bin
ENV VCPKG_ROOT=/usr/local/src/vcpkg

# Install external dependencies

RUN vcpkg install nlohmann-json gtest

# Install vulkan tools
RUN apt-get -y install vulkan-tools
#RUN vulkan-sdk-components glslang[tools]

# Install Qt
RUN apt-get -y install       \
        '^libxcb.*-dev'      \
        libx11-xcb-dev       \
        libgl1-mesa-dev      \
        libxrender-dev       \
        libxi-dev            \
        libxkbcommon-dev     \
        libxkbcommon-x11-dev \
  && apt-get -y install  \
        autoconf         \
        automake         \
        autoconf-archive \
        libtool          \
        libsystemd-dev   \
        libarchive-dev   \
        libxrandr-dev    \
        python3-pip      \
  && pip install Jinja2
RUN vcpkg install qtbase[vulkan]

ARG USERNAME=tim
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN    groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

USER $USERNAME

# install xxd
