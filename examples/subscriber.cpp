#include "HelloWorld.hpp"
#include "HelloWorldPubSubTypes.hpp"

#include <chrono>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fstream>
#include <thread>

using namespace eprosima::fastdds::dds;

class SubListener : public DataReaderListener {
  public:
    SubListener() : samples_(0) {}

    ~SubListener() override {}

    void on_subscription_matched(DataReader *, const SubscriptionMatchedStatus &info) override {
        if (info.current_count_change == 1) {
            std::cout << "Subscriber matched." << std::endl;
        } else if (info.current_count_change == -1) {
            std::cout << "Subscriber unmatched." << std::endl;
        } else {
            std::cout << info.current_count_change
                      << " is not a valid value for SubscriptionMatchedStatus current count change."
                      << std::endl;
        }
    }

    void on_data_available(DataReader *reader) override {
        SampleInfo info;
        HelloWorld hello;
        while (reader->take_next_sample(&hello, &info) == RETCODE_OK) {
            if (info.valid_data) {
                samples_++;
                std::string msg = "Message: " + hello.message() +
                                  " with index: " + std::to_string(hello.index()) + " RECEIVED.\n";
                std::cout << msg;

                // Log to file
                std::ofstream logfile("/tmp/subscriber.log", std::ios::app);
                if (logfile.is_open()) {
                    logfile << msg;
                    logfile.close();
                }
            }
        }
    }

    std::atomic<int> samples_;
};

class HelloWorldSubscriber {
  private:
    DomainParticipant *participant_;
    Subscriber *subscriber_;
    DataReader *reader_;
    Topic *topic_;
    TypeSupport type_;
    SubListener listener_;

  public:
    HelloWorldSubscriber()
        : participant_(nullptr), subscriber_(nullptr), topic_(nullptr), reader_(nullptr),
          type_(new HelloWorldPubSubType()) {}

    ~HelloWorldSubscriber() {
        if (reader_ != nullptr) {
            subscriber_->delete_datareader(reader_);
        }
        if (topic_ != nullptr) {
            participant_->delete_topic(topic_);
        }
        if (subscriber_ != nullptr) {
            participant_->delete_subscriber(subscriber_);
        }
        if (participant_ != nullptr) {
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
        }
    }

    bool init() {
        DomainParticipantQos participantQos;
        participantQos.name("Participant_subscriber");
        participant_ =
            DomainParticipantFactory::get_instance()->create_participant(0, participantQos);

        if (participant_ == nullptr) {
            std::cerr << "Failed to create participant" << std::endl;
            return false;
        }
        std::cout << "Participant created" << std::endl;

        type_.register_type(participant_);
        std::cout << "Type registered" << std::endl;

        topic_ = participant_->create_topic("HelloWorldTopic", "HelloWorld", TOPIC_QOS_DEFAULT);

        if (topic_ == nullptr) {
            std::cerr << "Failed to create topic" << std::endl;
            return false;
        }
        std::cout << "Topic created" << std::endl;

        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);

        if (subscriber_ == nullptr) {
            std::cerr << "Failed to create subscriber" << std::endl;
            return false;
        }
        std::cout << "Subscriber created" << std::endl;

        reader_ = subscriber_->create_datareader(topic_, DATAREADER_QOS_DEFAULT, &listener_);

        if (reader_ == nullptr) {
            std::cerr << "Failed to create reader" << std::endl;
            return false;
        }
        std::cout << "Reader created" << std::endl;

        return true;
    }

    void run(uint32_t samples) {
        while (listener_.samples_ < samples) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
};

int main(int argc, char **argv) {
    // Clear the log file
    std::ofstream logfile("/tmp/subscriber.log", std::ios::trunc);
    if (logfile.is_open()) {
        logfile << "=== Subscriber started ===\n";
        logfile.close();
    }

    std::cout << "Starting subscriber." << std::endl;
    int samples = 10;

    HelloWorldSubscriber subscriber;
    if (subscriber.init()) {
        std::cout << "Subscriber initialized, waiting for messages..." << std::endl;
        subscriber.run(static_cast<uint32_t>(samples));
    }

    std::cout << "Subscriber exiting." << std::endl;
    return 0;
}
