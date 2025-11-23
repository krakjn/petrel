# Petrel IDL Examples

This directory includes IDL-based examples that follow the eProsima Fast-DDS hello_world pattern more directly.

## IDL Approach

The IDL (Interface Definition Language) approach uses Fast-DDS-Gen to generate type support code from an IDL file, which is the standard way to define data types in Fast-DDS.

### Files

- **HelloWorld.idl**: The IDL definition of the message structure
- **gen/**: Directory containing all IDL-generated files (auto-generated, git-ignored)
- **examples/publisher.cpp**: Simple publisher using the IDL-generated types
- **examples/subscriber.cpp**: Simple subscriber using the IDL-generated types

### HelloWorld IDL

```idl
struct HelloWorld
{
    unsigned long index;
    string message;
};
```

### Building

The IDL files are automatically processed during the Docker image build:

```bash
# Rebuild Docker image with Fast-DDS-Gen and just
just img

# Build the project
just docker-build
```

### Generating Type Support from IDL

To regenerate the type support code from IDL files:

```bash
just gen-types
```

### Running

```bash
# Test the IDL examples
just test-idl

# Or manually using just in the container:
# Terminal 1:
docker run --rm -v "$(pwd):/workspace" -w /workspace petrel-builder ./build/subscriber

# Terminal 2:
docker run --rm -v "$(pwd):/workspace" -w /workspace petrel-builder ./build/publisher
```

### How It Works

1. **IDL Definition**: `HelloWorld.idl` defines a simple message structure
2. **Code Generation**: `just gen-types` runs `fastddsgen` to generate type support code into `gen/` directory
3. **Publisher**: Creates a DomainParticipant, registers the type, creates a topic, and publishes messages
4. **Subscriber**: Creates a DomainParticipant, registers the type, creates a topic, and receives messages via a listener

### Generated Files

All generated files are placed in the `gen/` directory:
- `gen/HelloWorld.hpp` - C++ type definition
- `gen/HelloWorldCdrAux.hpp/ipp` - CDR serialization helpers
- `gen/HelloWorldPubSubTypes.cxx/hpp` - Fast-DDS type support
- `gen/HelloWorldTypeObjectSupport.cxx/hpp` - Type object support

This directory is git-ignored and should be regenerated using `just gen-types`.

### Output

When running the test, you should see:

```
Starting subscriber.
Participant created
Type registered
Topic created
Subscriber created
Reader created
Subscriber initialized, waiting for messages...

Starting publisher.
Participant created
Type registered
Topic created
Publisher created
Writer created
Publisher initialized, waiting for subscriber to match...
Subscriber matched.
Starting to publish messages...
Message: Hello World with index: 0 SENT
Message: Hello World with index: 1 SENT
...
Message: Hello World with index: 0 RECEIVED.
Message: Hello World with index: 1 RECEIVED.
...
```

## Differences from libpetrel

The IDL examples demonstrate direct Fast-DDS usage, while `libpetrel` provides a simplified wrapper that:

- Automatically handles type registration
- Uses byte arrays for message content (like MQTT)
- Prefixes topics with `petrel/`
- Provides a simpler API (`init()`, `publish()`, `subscribe()`)

Choose the IDL approach when:
- You need structured message types with fields
- You want full control over Fast-DDS features
- You're already familiar with Fast-DDS patterns

Choose `libpetrel` when:
- You want a simple pub/sub API
- You prefer byte array messages
- You want automatic topic prefixing

## Just Commands

All scripts have been integrated into the justfile for convenience:

- `just gen-types` - Regenerate type support code from IDL files
- `just test-idl` - Run the IDL pub/sub test
- `just docker-build` - Build the project in Docker
- `just --list` - Show all available commands

See `JUSTFILE_GUIDE.md` for more details on using just.

