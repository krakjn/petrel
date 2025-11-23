#include "petrel.h"
/** these are auto-generated from the IDL file */
#include "HelloWorld.hpp"
#include "HelloWorldPubSubTypes.hpp"

#include <chrono>
#include <iostream>
#include <thread>

template <> struct petrel::Message<HelloWorld> {
    using type = HelloWorldPubSubType;
};

int main(int argc, char **argv) {
    std::cout << "Starting publisher." << std::endl;

    petrel::Publisher<HelloWorld> publisher;

    if (!publisher.init()) {
        std::cerr << "Failed to initialize publisher" << std::endl;
        return 1;
    }

    std::cout << "Publisher initialized, waiting for subscriber..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "Publishing messages..." << std::endl;

    for (int i = 0; i < 10; ++i) {
        HelloWorld msg;
        msg.index(i);
        msg.message("Hello World");

        if (publisher.publish("HelloWorldTopic", msg)) {
            std::cout << "Published: " << msg.message() << " [" << msg.index() << "]" << std::endl;
        } else {
            std::cerr << "Failed to publish message" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "All messages sent." << std::endl;
    publisher.stop();

    return 0;
}
