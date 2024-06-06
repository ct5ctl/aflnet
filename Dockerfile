# syntax=docker/dockerfile-upstream:master-labs
FROM ubuntu:18.04

# 设置环境变量
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get -y install sudo \
    apt-utils \
    build-essential \
    openssl \
    llvm clang \
    graphviz-dev \
    git \
    wget \
    curl \
    libglib2.0-dev \
    libpixman-1-dev \
    libssl-dev \
    libxml2-dev \
    zlib1g-dev \
    python3 \
    python3-pip \
    libcap-dev \
    gdb


# Download and compile AFLNet
ENV LLVM_CONFIG="llvm-config-6.0"

# 克隆 AFLNET 项目
WORKDIR /opt
RUN git clone https://github.com/ct5ctl/aflnet.git
WORKDIR /opt/aflnet

RUN make clean all && \
    cd llvm_mode && \
    make

# Set up environment variables for AFLNet
ENV AFLNET="/opt/aflnet"
ENV PATH="${PATH}:${AFLNET}"
ENV AFL_PATH="${AFLNET}"
ENV AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1 \
    AFL_SKIP_CPUFREQ=1
