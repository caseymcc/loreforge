# Use a standard Ubuntu image as a base
FROM ubuntu:22.04

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    wget \
    gnupg \
    software-properties-common && \
    rm -rf /var/lib/apt/lists/*

RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | \
    gpg --dearmor | \
    tee /etc/apt/trusted.gpg.d/llvm-snapshot.gpg > /dev/null

ARG UBUNTU_CODENAME
RUN if [ -z "$UBUNTU_CODENAME" ]; then \
        UBUNTU_CODENAME=$(lsb_release -sc); \
    fi && \
    echo "deb http://apt.llvm.org/$UBUNTU_CODENAME/ llvm-toolchain-$UBUNTU_CODENAME-17 main" >> /etc/apt/sources.list.d/llvm.list

# Install essential build tools and dependencies
RUN apt-get update && \
    apt-get install -y build-essential git cmake ninja-build curl zip unzip pkg-config gfortran
#RUN apt-get install -y clang-12
RUN apt-get install -y \
    clang-17 \
    lld-17 \
    clang-format-17 \
    llvm-17-dev \
    libclang-17-dev \
    libomp-17-dev

RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-17 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-17 100

ENV CC=/usr/bin/clang
ENV CXX=/usr/bin/clang++

# Set up the working directory for the project
WORKDIR /app

# Clone and install vcpkg
RUN git clone https://github.com/Microsoft/vcpkg.git /opt/vcpkg && \
    /opt/vcpkg/bootstrap-vcpkg.sh

# Set the default command to start a bash shell
CMD ["/bin/bash"]