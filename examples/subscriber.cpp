#include "petrel.h"
/** these are auto-generated from the IDL file */
#include "HelloWorld.hpp"
#include "HelloWorldPubSubTypes.hpp"

#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

template <> struct petrel::Message<HelloWorld> {
    using type = HelloWorldPubSubType;
};

int main(int argc, char **argv) {
    // Clear the log file
    std::ofstream logfile("/tmp/subscriber.log", std::ios::trunc);
    if (logfile.is_open()) {
        logfile << "=== Subscriber started ===\n";
        logfile.close();
    }

    std::cout << "Starting subscriber." << std::endl;

    std::atomic<int> message_count(0);

    petrel::Subscriber<HelloWorld> subscriber;

    if (!subscriber.init([&message_count](const std::string &topic, const HelloWorld &msg) {
            message_count++;
            std::string output = "Message: " + msg.message() +
                                 " with index: " + std::to_string(msg.index()) + " RECEIVED.\n";
            std::cout << output;

            // Log to file
            std::ofstream logfile("/tmp/subscriber.log", std::ios::app);
            if (logfile.is_open()) {
                logfile << output;
                logfile.close();
            }
        })) {
        std::cerr << "Failed to initialize subscriber" << std::endl;
        return 1;
    }

    if (!subscriber.subscribe("HelloWorldTopic")) {
        std::cerr << "Failed to subscribe to topic" << std::endl;
        return 1;
    }

    std::cout << "Subscriber initialized, waiting for messages..." << std::endl;

    // Wait for 10 messages
    while (message_count < 10) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Subscriber exiting." << std::endl;
    subscriber.stop();

    return 0;
}
