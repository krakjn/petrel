FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    curl \
    default-jre \
    git \
    libasio-dev \
    libengine-pkcs11-openssl \
    libp11-kit-dev \
    libssl-dev \
    libtinyxml2-dev \
    libxml2-dev \
    wget \
    && rm -rf /var/lib/apt/lists/*


# Install FastCDR (dependency of Fast-DDS)
RUN <<EOF
git clone -b v2.3.4 https://github.com/eProsima/Fast-CDR.git cdr
cmake -S cdr -B cdr/build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build cdr/build --target install
rm -rf cdr
EOF

# Install foonathan_memory (dependency of Fast-DDS)
RUN <<EOF
git clone -b v0.7-4 https://github.com/foonathan/memory.git foonathan
cmake -S foonathan -B foonathan/build -DCMAKE_INSTALL_PREFIX=/usr -DFOONATHAN_MEMORY_BUILD_EXAMPLES=OFF -DFOONATHAN_MEMORY_BUILD_TESTS=OFF
cmake --build foonathan/build --target install
rm -rf foonathan
EOF

# Install Fast-DDS, need to build as eprosima doesn't ship prebuilt packages
RUN <<EOF
git clone -b v3.4.1 --recursive https://github.com/eProsima/Fast-DDS.git dds
cmake -S dds -B dds/build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build dds/build --target install
rm -rf dds
EOF

# Install Fast-DDS-Gen (IDL code generator)
RUN <<EOF
git clone -b v4.2.0 --recursive https://github.com/eProsima/Fast-DDS-Gen.git fastddsgen
cd fastddsgen
./gradlew assemble
cp -r share/fastddsgen /usr/local/share/
cp scripts/fastddsgen /usr/local/bin/
chmod +x /usr/local/bin/fastddsgen
cd ..
rm -rf fastddsgen
EOF

# Install just command runner
RUN curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | bash -s -- --to /usr/local/bin

CMD ["/bin/bash"]

