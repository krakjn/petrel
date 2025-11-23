# petrel

Fast-DDS conventionalized for ease of use

## Overview

Petrel is a C++ wrapper library around Fast-DDS that provides a simple, object-oriented API for pub/sub messaging. All topics are automatically prefixed with `petrel/` and messages are handled as byte arrays (`std::vector<uint8_t>`), similar to MQTT.

The library follows the Fast-DDS hello world example pattern with separate `Publisher` and `Subscriber` classes, making it easy to use and understand.

## Features

- Simple object-oriented API following Fast-DDS patterns: `Publisher::init()`, `Publisher::publish()`, `Subscriber::init()`, `Subscriber::subscribe()`
- Automatic topic prefixing with `petrel/`
- Byte array messages (like MQTT)
- Separate Publisher and Subscriber classes for clean separation of concerns
- Docker-based build system with `just` command

## Building

### Prerequisites

- Docker
- [Just](https://github.com/casey/just) command runner (installed both locally and in the Docker container)

### Build

Local build (requires Fast-DDS installed locally):
```bash
just build
```

Docker build (recommended):
```bash
# First time: build the Docker image
just img

# Then build the project
just docker-build
```

This will:
1. Use a Docker container with Fast-DDS and just installed
2. Mount the current directory into the container
3. Build the project using CMake inside the container
4. Output build artifacts to `build/` directory

### Run Examples

```bash
# Basic usage example
just run

# Or manually:
./build/examples/basic_usage

# Hello world example (separate publisher/subscriber)
# Terminal 1:
./build/hello_world subscriber

# Terminal 2:
./build/hello_world publisher
```

## Usage

### Basic Usage

```cpp
#include "petrel/petrel.hpp"
#include <vector>
#include <iostream>

int main() {
    // Create and initialize publisher
    petrel::Publisher publisher;
    if (!publisher.init()) {
        std::cerr << "Failed to initialize publisher" << std::endl;
        return 1;
    }
    
    // Create and initialize subscriber with callback
    petrel::Subscriber subscriber;
    if (!subscriber.init([](const std::string& topic, const std::vector<uint8_t>& data) {
        std::cout << "Received on " << topic << ": ";
        for (uint8_t byte : data) {
            std::cout << static_cast<char>(byte);
        }
        std::cout << std::endl;
    })) {
        std::cerr << "Failed to initialize subscriber" << std::endl;
        return 1;
    }
    
    // Subscribe to topic (automatically prefixed with "petrel/")
    subscriber.subscribe("mytopic");  // Subscribes to "petrel/mytopic"
    
    // Publish message
    std::vector<uint8_t> message = {'H', 'e', 'l', 'l', 'o'};
    publisher.publish("mytopic", message);  // Publishes to "petrel/mytopic"
    
    // Cleanup
    publisher.stop();
    subscriber.stop();
    
    return 0;
}
```

### Hello World Pattern

Following the Fast-DDS hello world example pattern:

```cpp
#include "petrel/petrel.hpp"

class HelloWorldPublisher {
public:
    HelloWorldPublisher() {}
    
    bool init() {
        return publisher_.init();
    }
    
    bool publish() {
        std::string message = "Hello World";
        std::vector<uint8_t> data(message.begin(), message.end());
        return publisher_.publish("HelloWorldTopic", data);
    }
    
private:
    petrel::Publisher publisher_;
};

class HelloWorldSubscriber {
public:
    HelloWorldSubscriber() {}
    
    bool init() {
        subscriber_.init([this](const std::string& topic, const std::vector<uint8_t>& data) {
            // Handle received message
        });
        return subscriber_.subscribe("HelloWorldTopic");
    }
    
private:
    petrel::Subscriber subscriber_;
};
```

## API Reference

### `petrel::Publisher`

#### Constructor
```cpp
petrel::Publisher();
```
Creates a new publisher instance.

#### Methods

##### `bool init()`
Initializes the publisher and creates the Fast-DDS participant and publisher. Returns `true` on success.

##### `bool publish(const std::string& topic, const std::vector<uint8_t>& data)`
Publishes a message to the specified topic. The topic is automatically prefixed with `petrel/` if not already present. Returns `true` on success.

##### `void stop()`
Stops the publisher and cleans up all resources.

### `petrel::Subscriber`

#### Constructor
```cpp
petrel::Subscriber();
```
Creates a new subscriber instance.

#### Methods

##### `bool init(std::function<void(const std::string& topic, const std::vector<uint8_t>& data)> callback)`
Initializes the subscriber with a callback function and creates the Fast-DDS participant and subscriber. The callback receives the topic name and message data. Returns `true` on success.

##### `bool subscribe(const std::string& topic)`
Subscribes to the specified topic. The topic is automatically prefixed with `petrel/` if not already present. Returns `true` on success.

##### `void stop()`
Stops the subscriber and cleans up all resources.

## License

Apache License 2.0
