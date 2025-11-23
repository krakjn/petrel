# Justfile Guide

This project uses [just](https://github.com/casey/just) as a command runner, both locally and inside the Docker container.

## Installation

### Local (macOS/Linux)
```bash
curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | bash -s -- --to /usr/local/bin
```

### Docker
Just is automatically installed in the `petrel-builder` Docker image.

## Available Recipes

Run `just --list` to see all available recipes:

```
$ just --list
Available recipes:
    build        # Build the project locally
    clean        # Clean build artifacts
    docker-build # Build the project using Docker
    gen-types    # Generate type support code from IDL files
    img          # Build Docker image with Fast-DDS and just
    run          # Run the basic_usage example
    sh           # Open a shell in the Docker container
    test         # Test - run hello world publisher and subscriber (uses libpetrel wrapper)
    test-idl     # Test IDL-based publisher/subscriber
```

## Recipe Details

### Build Recipes

- **`just build`**: Build locally (requires Fast-DDS installed)
- **`just docker-build`**: Build using Docker container
- **`just img`**: Build/rebuild the Docker image
- **`just clean`**: Remove build artifacts

### Test Recipes

- **`just test`**: Test the libpetrel wrapper (hello_world example)
- **`just test-idl`**: Test the IDL-based publisher/subscriber

### IDL Recipes

- **`just gen-types`**: Generate type support code from `HelloWorld.idl` into `gen/` directory

### Utility Recipes

- **`just sh`**: Open an interactive shell inside the Docker container
- **`just run`**: Run the basic_usage example

## How It Works

The justfile uses a pattern where public recipes call Docker, which then calls internal recipes inside the container:

```
just docker-build
  └─> docker run ... just _build-internal
       └─> cmake -B build -S . && cmake --build build
```

Internal recipes (prefixed with `_`) are meant to run inside the container and should not be called directly from the host.

## Typical Workflow

1. **First time setup:**
   ```bash
   just img          # Build Docker image
   just docker-build # Build the project
   ```

2. **After code changes:**
   ```bash
   just docker-build # Rebuild
   just test-idl     # Test
   ```

3. **After IDL changes:**
   ```bash
   just gen-types    # Regenerate type support into gen/
   just docker-build # Rebuild
   just test-idl     # Test
   ```

## Project Structure

```
petrel/
├── HelloWorld.idl          # IDL source (committed to git)
├── gen/                    # Generated files (git-ignored)
│   ├── HelloWorld.hpp
│   ├── HelloWorldPubSubTypes.cxx/hpp
│   └── HelloWorldTypeObjectSupport.cxx/hpp
├── examples/               # Example programs
│   ├── publisher.cpp
│   └── subscriber.cpp
├── src/                    # libpetrel source
├── include/                # libpetrel headers
└── build/                  # Build artifacts (git-ignored)
```

4. **Debugging:**
   ```bash
   just sh           # Open shell in container
   # Inside container:
   just _build-internal
   ./build/publisher
   ```

## Benefits of This Approach

1. **Consistency**: Same tool (just) used on host and in container
2. **Simplicity**: Short, memorable commands
3. **Documentation**: Recipes are self-documenting
4. **Flexibility**: Can run commands locally or in Docker with the same interface
5. **Composability**: Recipes can call other recipes

