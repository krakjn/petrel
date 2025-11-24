# Petrel

```
┏━┃┏━┛━┏┛┏━┃┏━┛┃  
┏━┛┏━┛ ┃ ┏┏┛┏━┛┃  
┛  ━━┛ ┛ ┛ ┛━━┛━━┛
 Fast-DDS, Simplified
```

A clean, type-safe C++ wrapper around eProsima Fast-DDS that makes pub/sub messaging simple and intuitive.

## Overview

Petrel provides a modern template-based API for Fast-DDS, eliminating boilerplate while maintaining full type safety. Define your messages in IDL, and get a clean, easy-to-use publisher/subscriber interface.

**Key Features:**
- Simple API, Just `Publisher<T>` and `Subscriber<T>`
- User defined Message types `<T>` 
- Interface Domain Language based, Define messages `.idl` once, use everywhere
- Single header, for portability


## Quick Start

### Prerequisites

- Docker
- [Just](https://github.com/casey/just) command runner
- Java Runtime Environment (for `fastddsgen` IDL code generation)

### Building
> fast-dds is a heavier dep, `just img` will take some time

```bash
just img    # build the docker image
just build  # build petrel
just test   # run the example
```

## Usage

### Installation

**Prerequisites:**
- Java Runtime Environment (JRE) 11 or higher - required for `fastddsgen` tool

**Linux (Debian/Ubuntu):**
```bash
# Install Java if not already installed
sudo apt-get install default-jre

# Install petrel
sudo dpkg -i petrel-0.1.0-linux-amd64.deb
```

**Linux (Generic/Tarball):**
```bash
tar -xzf petrel-0.1.0-linux-x86_64.tar.gz
sudo cp -r usr/* /usr/
```

**macOS:**
```bash
# Install Java if not already installed
brew install openjdk@11

# Install petrel
tar -xzf petrel-0.1.0-darwin-arm64.tar.gz
sudo cp -r usr/local/* /usr/local/
```

**Windows:**
```powershell
# Install Java from https://jdk.java.net/25/
# Extract petrel-0.1.0-windows-x86_64.zip
# Add C:\Program Files\petrel\bin to PATH
```

### 1. Define Your Message (IDL)

Create your message structure in an IDL file:

```idl
// MyMessage.idl
struct MyMessage
{
    unsigned long id;
    double value;
    string description;
};
```

### 2. Generate Bindings from `*.idl`

```bash
fastddsgen -replace -flat-output-dir MyMessage.idl
```

This generates:
- `MyMessage.hpp`
- `MyMessagePubSubTypes.hpp`
- `MyMessagePubSubTypes.cxx`
- `MyMessageTypeObjectSupport.hpp`
- `MyMessageTypeObjectSupport.cxx`

### 3. Write Your Publisher

```cpp
#include <petrel.h>
#include "MyMessage.hpp"
#include "MyMessagePubSubTypes.hpp"

// Register the message type
template <> struct petrel::Message<MyMessage> {
    using type = MyMessagePubSubType;
};

int main() {
    petrel::Publisher<MyMessage> pub;
    if (!pub.init()) {
        return 1;
    }
    
    MyMessage msg;
    msg.id(42);
    msg.value(3.14);
    msg.description("Hello from Petrel!");
    
    pub.publish("my/topic", msg);
    pub.stop();
    return 0;
}
```

### 4. Write Your Subscriber

```cpp
#include <petrel.h>
#include "MyMessage.hpp"
#include "MyMessagePubSubTypes.hpp"

// Register the message type
template <> struct petrel::Message<MyMessage> {
    using type = MyMessagePubSubType;
};

int main() {
    petrel::Subscriber<MyMessage> sub;
    
    sub.init([](const std::string& topic, const MyMessage& msg) {
        std::cout << "Received on " << topic << ": "
                  << "id=" << msg.id() << " "
                  << "value=" << msg.value() << " "
                  << "desc=" << msg.description() << std::endl;
    });
    
    sub.subscribe("my/topic");
    
    // Keep running
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    sub.stop();
    return 0;
}
```

### 5. Build Your Application

Create a `CMakeLists.txt` for your project:

```cmake
cmake_minimum_required(VERSION 3.10)
project(my_petrel_app)

set(CMAKE_CXX_STANDARD 17)

# Find Fast-DDS (bundled with petrel)
find_library(FASTDDS_LIB fastdds REQUIRED)
find_library(FASTCDR_LIB fastcdr REQUIRED)

# Your generated IDL sources
add_executable(publisher 
    publisher.cpp
    MyMessagePubSubTypes.cxx
    MyMessageTypeObjectSupport.cxx
)
target_link_libraries(publisher ${FASTDDS_LIB} ${FASTCDR_LIB})

add_executable(subscriber 
    subscriber.cpp
    MyMessagePubSubTypes.cxx
    MyMessageTypeObjectSupport.cxx
)
target_link_libraries(subscriber ${FASTDDS_LIB} ${FASTCDR_LIB})
```

Build:
```bash
cmake -B build
cmake --build build
```

## API Reference

### `petrel::Publisher<T>`

```cpp
template <typename T>
class Publisher {
public:
    Publisher();
    ~Publisher();
    
    // Initialize with optional domain ID
    bool init(int domain_id = 0);
    
    // Publish a message to a topic
    bool publish(const std::string& topic_name, const T& message);
    
    // Cleanup
    void stop();
};
```

### `petrel::Subscriber<T>`

```cpp
template <typename T>
class Subscriber {
public:
    Subscriber();
    ~Subscriber();
    
    // Initialize with callback
    bool init(std::function<void(const std::string&, const T&)> callback, 
              int domain_id = 0);
    
    // Subscribe to a topic
    bool subscribe(const std::string& topic_name);
    
    // Cleanup
    void stop();
};
```

### `petrel::Message<T>`

Type trait to map your IDL types to their PubSubType:

```cpp
template <> struct petrel::Message<YourType> {
    using type = YourTypePubSubType;
};
```

## Available Commands

```bash
just --list
```

## How It Works

### The IDL Workflow

1. **Define** your message structure in an IDL file
2. **Generate** C++ types using `fastddsgen`
3. **Specialize** `petrel::Message<>` trait for your type
4. **Use** type-safe `Publisher<T>` and `Subscriber<T>`


## Adding Custom Types

### Step 1: Create IDL

```idl
// examples/SensorData.idl
struct SensorData
{
    unsigned long long timestamp;
    unsigned long sensor_id;
    double temperature;
    string location;
};
```

### Step 2: Update justfile (if needed)

If using multiple IDL files, update the `gen-types` recipe.

### Step 3: Generate and Use

```bash
just gen-types
```

```cpp
#include "petrel.h"
#include "SensorData.hpp"
#include "SensorDataPubSubTypes.hpp"

template <> struct petrel::Message<SensorData> {
    using type = SensorDataPubSubType;
};

int main() {
    petrel::Publisher<SensorData> pub;
    pub.init();
    
    SensorData data;
    data.timestamp(/* ... */);
    data.sensor_id(42);
    data.temperature(25.3);
    data.location("Room-101");
    
    pub.publish("sensors/temperature", data);
    return 0;
}
```

## Architecture

### Design Principles

1. **Header-only templates** - Fast compilation, flexible usage
2. **Type safety first** - Catch errors at compile time
3. **Minimal API surface** - Easy to learn and use
4. **Standard C++17** - Modern, readable code
5. **Explicit resource management** - Clear initialization and cleanup

### Fast-DDS Integration

Petrel creates and manages Fast-DDS entities for you:

- **DomainParticipant** - One per publisher/subscriber
- **Publisher/Subscriber** - Created during `init()`
- **Topics** - Auto-created when publishing/subscribing
- **DataWriters/DataReaders** - One per topic
- **TypeSupport** - Registered automatically

## Code Formatting

The project uses clang-format with LLVM style:

```bash
just fmt
```

## Performance Considerations

- **Publisher reuse** - Create once, publish many times
- **Topic reuse** - Writers/readers are cached per topic
- **Callbacks** - Keep subscriber callbacks fast and non-blocking
- **Cleanup** - Always call `stop()` or use RAII destructors

## Contributing

Petrel follows a simple structure:
- `include/petrel.h` - Declarations then implementations
- Examples show the pattern
- All public API is in `petrel::` namespace
- Fast-DDS types aliased under `fastdds::`

## License

Apache License 2.0

## Acknowledgments

Built on [eProsima Fast-DDS](https://github.com/eProsima/Fast-DDS) - Industrial-strength DDS implementation.
