#ifndef PETREL_HPP
#define PETREL_HPP

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace petrel {

class Publisher
{
public:
    Publisher();
    ~Publisher();

    // Initialize the publisher
    bool init();

    // Publish a message to a topic (auto-prefixed with petrel/)
    bool publish(const std::string& topic, const std::vector<uint8_t>& data);

    // Cleanup/teardown
    void stop();

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

class Subscriber
{
public:
    Subscriber();
    ~Subscriber();

    // Initialize the subscriber with a callback
    bool init(std::function<void(const std::string& topic, const std::vector<uint8_t>& data)> callback);

    // Subscribe to a topic (auto-prefixed with petrel/)
    bool subscribe(const std::string& topic);

    // Cleanup/teardown
    void stop();

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace petrel

#endif // PETREL_HPP
