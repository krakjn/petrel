IMAGE := "ghcr.io/krakjn/petrel:0.1.0"

_docker_run CMD:
    #!/bin/bash
    docker run --rm -it \
        -v "{{justfile_directory()}}:$(pwd)" \
        -w $(pwd) \
        {{IMAGE}} {{CMD}}

# Build the project in the Docker container
build: (_docker_run 'just _cmake')

# Build Docker image with Fast-DDS and just
img:
    @docker build -t {{IMAGE}} -< Dockerfile

# Open a shell in the Docker container
sh: (_docker_run 'bash')

# Clean build artifacts
clean:
    rm -rf build gen

# Format all C++ source files with clang-format
fmt:
    #!/bin/bash
    find include examples -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -exec clang-format -i {} \;

# Test - run IDL publisher and subscriber
test: (_docker_run 'just _test-idl-internal')

# Internal recipe: build inside container
_cmake: _gen-types
    #!/bin/bash
    cmake -B build -S .
    cmake --build build


# Internal recipe: generate types inside container
_gen-types:
    #!/bin/bash
    mkdir -p gen
    fastddsgen -replace -d gen/ -flat-output-dir examples/HelloWorld.idl

# Internal recipe: test IDL inside container
_test-idl-internal:
    #!/bin/bash
    echo "Testing IDL-based publisher/subscriber..."
    echo
    rm -f /tmp/subscriber.log
    timeout 20 bash -c './build/subscriber & sleep 3 && ./build/publisher' || true
    echo ''
    echo '=== MESSAGES RECEIVED ==='
    cat /tmp/subscriber.log

# Internal recipe: build packages inside container
package: _gen-types
    #!/bin/bash
    cmake -B build -S . -DCMAKE_INSTALL_PREFIX=/usr
    cmake --build build
    cd build
    echo "Creating DEB package..."
    cpack -G DEB
    echo "Creating TGZ package..."
    cpack -G TGZ

