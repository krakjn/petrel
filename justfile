# Build the project locally
build:
    cmake -B build -S . && cmake --build build

# Build the project using Docker
docker-build:
    #!/bin/bash
    docker run --rm \
        -v "{{justfile_directory()}}:$(pwd)" \
        -w $(pwd) \
        petrel-builder just _build-internal

# Build Docker image with Fast-DDS and just
img:
    #!/bin/bash
    docker build -t petrel-builder -f Dockerfile .

# Open a shell in the Docker container
sh:
    #!/bin/bash
    docker run --rm -it \
        -v "{{justfile_directory()}}:$(pwd)" \
        -w $(pwd) \
        petrel-builder bash

# Clean build artifacts
clean:
    rm -rf build

# Format all C++ source files with clang-format
fmt:
    #!/bin/bash
    find src include examples -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -exec clang-format -i {} \;

# Run publisher and subscriber in Docker (convenience command)
run:
    #!/bin/bash
    docker run --rm \
        -v "{{justfile_directory()}}:$(pwd)" \
        -w $(pwd) \
        petrel-builder bash -c './build/subscriber & sleep 2 && ./build/publisher; wait'

# Test - run IDL publisher and subscriber
test:
    #!/bin/bash
    docker run --rm \
        -v "{{justfile_directory()}}:$(pwd)" \
        -w $(pwd) \
        petrel-builder just _test-idl-internal

# Internal recipe: build inside container
_build-internal:
    cmake -B build -S . && cmake --build build

# Generate type support code from IDL files
gen-types:
    #!/bin/bash
    docker run --rm \
        -v "{{justfile_directory()}}:$(pwd)" \
        -w $(pwd) \
        petrel-builder just _gen-types-internal

# Test IDL-based publisher/subscriber (alias for test)
test-idl: test

# Internal recipe: generate types inside container
_gen-types-internal:
    #!/bin/bash
    echo "Generating type support from HelloWorld.idl..."
    mkdir -p gen
    fastddsgen -replace -d gen HelloWorld.idl
    echo "Type support generated successfully in gen/!"
    echo "Files created:"
    echo "  - gen/HelloWorld.hpp"
    echo "  - gen/HelloWorldCdrAux.hpp/ipp"
    echo "  - gen/HelloWorldPubSubTypes.cxx/hpp"
    echo "  - gen/HelloWorldTypeObjectSupport.cxx/hpp"

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

