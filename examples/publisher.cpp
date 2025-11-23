#include "HelloWorld.hpp"
#include "HelloWorldPubSubTypes.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/core/ReturnCode.hpp>

#include <chrono>
#include <thread>

using namespace eprosima::fastdds::dds;

class HelloWorldPublisher
{
private:
    HelloWorld hello_;
    DomainParticipant* participant_;
    Publisher* publisher_;
    Topic* topic_;
    DataWriter* writer_;
    TypeSupport type_;

public:
    HelloWorldPublisher()
        : participant_(nullptr)
        , publisher_(nullptr)
        , topic_(nullptr)
        , writer_(nullptr)
        , type_(new HelloWorldPubSubType())
    {
    }

    ~HelloWorldPublisher()
    {
        if (writer_ != nullptr)
        {
            publisher_->delete_datawriter(writer_);
        }
        if (publisher_ != nullptr)
        {
            participant_->delete_publisher(publisher_);
        }
        if (topic_ != nullptr)
        {
            participant_->delete_topic(topic_);
        }
        if (participant_ != nullptr)
        {
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
        }
    }

    bool init()
    {
        hello_.index(0);
        hello_.message("Hello World");

        DomainParticipantQos participantQos;
        participantQos.name("Participant_publisher");
        participant_ = DomainParticipantFactory::get_instance()->create_participant(0, participantQos);

        if (participant_ == nullptr)
        {
            std::cerr << "Failed to create participant" << std::endl;
            return false;
        }
        std::cout << "Participant created" << std::endl;

        type_.register_type(participant_);
        std::cout << "Type registered" << std::endl;

        topic_ = participant_->create_topic("HelloWorldTopic", "HelloWorld", TOPIC_QOS_DEFAULT);

        if (topic_ == nullptr)
        {
            std::cerr << "Failed to create topic" << std::endl;
            return false;
        }
        std::cout << "Topic created" << std::endl;

        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);

        if (publisher_ == nullptr)
        {
            std::cerr << "Failed to create publisher" << std::endl;
            return false;
        }
        std::cout << "Publisher created" << std::endl;

        writer_ = publisher_->create_datawriter(topic_, DATAWRITER_QOS_DEFAULT, nullptr);

        if (writer_ == nullptr)
        {
            std::cerr << "Failed to create writer" << std::endl;
            return false;
        }
        std::cout << "Writer created" << std::endl;
        return true;
    }

    bool publish()
    {
        ReturnCode_t ret = writer_->write(&hello_);
        if (ret == RETCODE_OK)
        {
            return true;
        }
        std::cerr << "Write failed with return code: " << static_cast<int>(ret) << std::endl;
        return false;
    }

    void run(uint32_t samples)
    {
        for (uint32_t i = 0; i < samples; ++i)
        {
            if (!publish())
            {
                break;
            }

            std::cout << "Message: " << hello_.message() << " with index: " << hello_.index()
                      << " SENT" << std::endl;
            hello_.index(hello_.index() + 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
};

int main(int argc, char** argv)
{
    std::cout << "Starting publisher." << std::endl;
    int samples = 10;

    HelloWorldPublisher publisher;
    if (publisher.init())
    {
        std::cout << "Publisher initialized, waiting for subscriber to match..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "Starting to publish messages..." << std::endl;
        publisher.run(static_cast<uint32_t>(samples));
        std::cout << "All messages sent." << std::endl;
    }

    return 0;
}

