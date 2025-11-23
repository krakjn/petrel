FROM ubuntu:22.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build tools and Fast-DDS dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    libasio-dev \
    libtinyxml2-dev \
    libssl-dev \
    libp11-kit-dev \
    libengine-pkcs11-openssl \
    libxml2-dev \
    && rm -rf /var/lib/apt/lists/*


# Install FastCDR (dependency of Fast-DDS) from source
WORKDIR /tmp
RUN git clone https://github.com/eProsima/Fast-CDR.git && \
    cd Fast-CDR && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr && \
    cmake --build . --target install && \
    cd / && rm -rf /tmp/Fast-CDR

# Install foonathan_memory (dependency of Fast-DDS) from source
WORKDIR /tmp
RUN git clone https://github.com/foonathan/memory.git foonathan_memory && \
    cd foonathan_memory && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DFOONATHAN_MEMORY_BUILD_EXAMPLES=OFF -DFOONATHAN_MEMORY_BUILD_TESTS=OFF && \
    cmake --build . --target install && \
    cd / && rm -rf /tmp/foonathan_memory

# Install Fast-DDS from source
WORKDIR /tmp
RUN git clone --recursive https://github.com/eProsima/Fast-DDS.git && \
    cd Fast-DDS && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr && \
    cmake --build . --target install && \
    cd / && rm -rf /tmp/Fast-DDS

# Install Fast-DDS-Gen (IDL code generator)
RUN apt-get update && apt-get install -y default-jre && rm -rf /var/lib/apt/lists/*
WORKDIR /tmp
RUN git clone --recursive https://github.com/eProsima/Fast-DDS-Gen.git && \
    cd Fast-DDS-Gen && \
    ./gradlew assemble && \
    mkdir -p /usr/local/share/fastddsgen && \
    cp -r share/fastddsgen/* /usr/local/share/fastddsgen/ && \
    cp scripts/fastddsgen /usr/local/bin/ && \
    chmod +x /usr/local/bin/fastddsgen && \
    cd / && rm -rf /tmp/Fast-DDS-Gen

# Install just command runner
RUN curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | bash -s -- --to /usr/local/bin

WORKDIR /workspace

