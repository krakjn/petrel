#include <map>
#include <vector>
#include <mutex>
#include <string>
#include <memory>
#include <cstring>
#include <iostream>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/core/status/SubscriptionMatchedStatus.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/core/detail/DDSReturnCode.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/topic/TopicDataType.hpp>

#include "petrel.h"

using namespace eprosima::fastdds::dds;
using eprosima::fastdds::dds::ReturnCode_t;
using eprosima::fastdds::dds::RETCODE_OK;
typedef eprosima::fastdds::dds::Publisher FastDDSPublisher;
typedef eprosima::fastdds::dds::Subscriber FastDDSSubscriber;

namespace petrel {

// Byte array type for Fast-DDS
class ByteArrayType : public TopicDataType {
public:
    ByteArrayType() {
        set_name("ByteArray");
    }

    bool serialize(
            const void* const data,
            eprosima::fastdds::rtps::SerializedPayload_t& payload,
            eprosima::fastdds::dds::DataRepresentationId_t /*data_representation*/) override {
        const auto* byte_array = static_cast<const std::vector<uint8_t>*>(data);
        payload.length = static_cast<uint32_t>(byte_array->size());
        payload.data = new uint8_t[payload.length];
        memcpy(payload.data, byte_array->data(), payload.length);
        return true;
    }

    bool deserialize(
            eprosima::fastdds::rtps::SerializedPayload_t& payload,
            void* data) override {
        auto* byte_array = static_cast<std::vector<uint8_t>*>(data);
        byte_array->resize(payload.length);
        memcpy(byte_array->data(), payload.data, payload.length);
        return true;
    }

    uint32_t calculate_serialized_size(
            const void* const data,
            eprosima::fastdds::dds::DataRepresentationId_t /*data_representation*/) override {
        const auto* byte_array = static_cast<const std::vector<uint8_t>*>(data);
        return static_cast<uint32_t>(byte_array->size());
    }

    void* create_data() override {
        return new std::vector<uint8_t>();
    }

    void delete_data(void* data) override {
        delete static_cast<std::vector<uint8_t>*>(data);
    }

    bool compute_key(
            eprosima::fastdds::rtps::SerializedPayload_t& /*payload*/,
            eprosima::fastdds::rtps::InstanceHandle_t& /*ihandle*/,
            bool /*force_md5*/) override {
        return false;
    }

    bool compute_key(
            const void* const /*data*/,
            eprosima::fastdds::rtps::InstanceHandle_t& /*ihandle*/,
            bool /*force_md5*/) override {
        return false;
    }
};

// DataReader listener
class PetrelDataReaderListener : public DataReaderListener {
public:
    PetrelDataReaderListener(
            std::function<void(const std::string&, const std::vector<uint8_t>&)> callback,
            const std::string& topic_name,
            TypeSupport type)
        : callback_(callback), topic_name_(topic_name), type_(type) {}

    void on_data_available(DataReader* reader) override {
        if (!callback_ || !type_ || !reader) return;

        SampleInfo info;
        std::vector<uint8_t>* data = static_cast<std::vector<uint8_t>*>(type_->create_data());
        
        if (reader->take_next_sample(data, &info) == RETCODE_OK) {
            if (info.valid_data && callback_) {
                callback_(topic_name_, *data);
            }
        }
        
        type_->delete_data(data);
    }

    void on_subscription_matched(DataReader*, const SubscriptionMatchedStatus&) override {}

private:
    std::function<void(const std::string&, const std::vector<uint8_t>&)> callback_;
    std::string topic_name_;
    TypeSupport type_;
};

// Publisher Implementation
class Publisher::Impl {
public:
    Impl() 
        : participant_(nullptr)
        , fastdds_publisher_(nullptr)
    {
    }

    ~Impl() {
        stop();
    }

    bool init() {
        DomainParticipantQos participant_qos;
        participant_ = DomainParticipantFactory::get_instance()->create_participant(
            0, participant_qos);
        
        if (!participant_) {
            std::cerr << "ERROR creating participant" << std::endl;
            return false;
        }

        // Register the type
        type_support_ = TypeSupport(new ByteArrayType());
        type_support_.register_type(participant_);

        // Create the publisher
        fastdds_publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
        
        if (!fastdds_publisher_) {
            std::cerr << "ERROR creating publisher" << std::endl;
            return false;
        }

        return true;
    }

    void stop() {
        if (!participant_) {
            return;
        }

        DomainParticipant* part = participant_;
        FastDDSPublisher* pub = fastdds_publisher_;

        // Clear pointers immediately to prevent reuse
        participant_ = nullptr;
        fastdds_publisher_ = nullptr;

        // Collect topics and writers before clearing map
        std::vector<Topic*> topics_to_delete;
        std::vector<DataWriter*> writers_to_delete;
        for (auto& [topic_name, writer_data] : writers_) {
            if (writer_data.topic) {
                topics_to_delete.push_back(writer_data.topic);
            }
            if (writer_data.writer) {
                writers_to_delete.push_back(writer_data.writer);
            }
        }
        writers_.clear();

        // Delete DataWriters
        if (pub) {
            for (DataWriter* writer : writers_to_delete) {
                if (writer) {
                    pub->delete_datawriter(writer);
                }
            }
        }

        // Delete Publisher
        if (part && pub) {
            part->delete_publisher(pub);
        }

        // Delete Topics
        if (part) {
            for (Topic* topic : topics_to_delete) {
                if (topic) {
                    part->delete_topic(topic);
                }
            }
        }

        // Delete DomainParticipant
        if (part) {
            DomainParticipantFactory::get_instance()->delete_participant(part);
        }
    }

    bool publish(const std::string& topic, const std::vector<uint8_t>& data) {
        if (!participant_ || !fastdds_publisher_) {
            return false;
        }

        std::string normalized_topic = normalize_topic(topic);
        
        // Get or create writer
        WriterData* writer_data = nullptr;
        auto it = writers_.find(normalized_topic);
        
        if (it == writers_.end()) {
            // Check if topic already exists
            TopicDescription* topic_desc = participant_->lookup_topicdescription(normalized_topic);
            Topic* fastdds_topic = nullptr;
            
            if (topic_desc) {
                fastdds_topic = dynamic_cast<Topic*>(topic_desc);
            }
            
            if (!fastdds_topic) {
                // Create topic
                fastdds_topic = participant_->create_topic(
                    normalized_topic, "ByteArray", TOPIC_QOS_DEFAULT);
                
                if (!fastdds_topic) {
                    std::cerr << "ERROR creating topic" << std::endl;
                    return false;
                }
            }

            // Create writer
            DataWriter* writer = fastdds_publisher_->create_datawriter(
                fastdds_topic, DATAWRITER_QOS_DEFAULT);
            
            if (!writer) {
                if (topic_desc == nullptr) {
                    participant_->delete_topic(fastdds_topic);
                }
                std::cerr << "ERROR creating writer" << std::endl;
                return false;
            }

            writer_data = &writers_[normalized_topic];
            writer_data->writer = writer;
            writer_data->topic = fastdds_topic;
        } else {
            writer_data = &it->second;
        }

        // Publish data
        if (!writer_data->writer) {
            return false;
        }
        
        std::vector<uint8_t> data_copy = data;
        ReturnCode_t ret = writer_data->writer->write(&data_copy);
        return ret == RETCODE_OK;
    }

private:
    std::string normalize_topic(const std::string& topic) {
        if (topic.find("petrel/") == 0) {
            return topic;
        }
        return "petrel/" + topic;
    }

    DomainParticipant* participant_;
    FastDDSPublisher* fastdds_publisher_;
    TypeSupport type_support_;

    struct WriterData {
        DataWriter* writer = nullptr;
        Topic* topic = nullptr;
    };

    std::map<std::string, WriterData> writers_;
};

// Subscriber Implementation
class Subscriber::Impl {
public:
    Impl() 
        : participant_(nullptr)
        , fastdds_subscriber_(nullptr)
    {
    }

    ~Impl() {
        stop();
    }

    bool init(std::function<void(const std::string&, const std::vector<uint8_t>&)> callback) {
        callback_ = callback;

        DomainParticipantQos participant_qos;
        participant_ = DomainParticipantFactory::get_instance()->create_participant(
            0, participant_qos);
        
        if (!participant_) {
            std::cerr << "ERROR creating participant" << std::endl;
            return false;
        }

        // Register the type
        type_support_ = TypeSupport(new ByteArrayType());
        type_support_.register_type(participant_);

        // Create the subscriber
        fastdds_subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
        
        if (!fastdds_subscriber_) {
            std::cerr << "ERROR creating subscriber" << std::endl;
            return false;
        }

        return true;
    }

    void stop() {
        if (!participant_) {
            return;
        }

        DomainParticipant* part = participant_;
        FastDDSSubscriber* sub = fastdds_subscriber_;

        // Clear pointers immediately to prevent reuse
        participant_ = nullptr;
        fastdds_subscriber_ = nullptr;

        // Collect topics, readers, and listeners before clearing map
        std::vector<Topic*> topics_to_delete;
        std::vector<DataReader*> readers_to_delete;
        std::vector<PetrelDataReaderListener*> listeners_to_delete;
        for (auto& [topic_name, reader_data] : readers_) {
            if (reader_data.topic) {
                topics_to_delete.push_back(reader_data.topic);
            }
            if (reader_data.reader) {
                readers_to_delete.push_back(reader_data.reader);
            }
            if (reader_data.listener) {
                listeners_to_delete.push_back(reader_data.listener);
            }
        }
        readers_.clear();

        // Delete DataReaders
        if (sub) {
            for (DataReader* reader : readers_to_delete) {
                if (reader) {
                    sub->delete_datareader(reader);
                }
            }
        }

        // Delete listeners
        for (PetrelDataReaderListener* listener : listeners_to_delete) {
            delete listener;
        }

        // Delete Subscriber
        if (part && sub) {
            part->delete_subscriber(sub);
            sub = nullptr; // Clear after deletion
        }

        // Delete Topics
        if (part) {
            for (Topic* topic : topics_to_delete) {
                if (topic) {
                    part->delete_topic(topic);
                }
            }
        }

        // Delete DomainParticipant
        if (part) {
            DomainParticipantFactory::get_instance()->delete_participant(part);
        }
    }

    bool subscribe(const std::string& topic) {
        if (!participant_ || !fastdds_subscriber_) {
            return false;
        }

        std::string normalized_topic = normalize_topic(topic);
        
        // Check if already subscribed
        if (readers_.find(normalized_topic) != readers_.end()) {
            return true;
        }

        // Check if topic already exists
        TopicDescription* topic_desc = participant_->lookup_topicdescription(normalized_topic);
        Topic* fastdds_topic = nullptr;
        
        if (topic_desc) {
            fastdds_topic = dynamic_cast<Topic*>(topic_desc);
        }
        
        if (!fastdds_topic) {
            // Create topic
            fastdds_topic = participant_->create_topic(
                normalized_topic, "ByteArray", TOPIC_QOS_DEFAULT);
            
            if (!fastdds_topic) {
                std::cerr << "ERROR creating topic" << std::endl;
                return false;
            }
        }

        // Create listener
        PetrelDataReaderListener* listener = new PetrelDataReaderListener(
            callback_, normalized_topic, type_support_);

        // Create reader
        DataReader* reader = fastdds_subscriber_->create_datareader(
            fastdds_topic, DATAREADER_QOS_DEFAULT, listener);
        
        if (!reader) {
            if (topic_desc == nullptr) {
                participant_->delete_topic(fastdds_topic);
            }
            delete listener;
            std::cerr << "ERROR creating reader" << std::endl;
            return false;
        }

        ReaderData reader_data;
        reader_data.reader = reader;
        reader_data.topic = fastdds_topic;
        reader_data.listener = listener;
        
        readers_[normalized_topic] = reader_data;
        return true;
    }

private:
    std::string normalize_topic(const std::string& topic) {
        if (topic.find("petrel/") == 0) {
            return topic;
        }
        return "petrel/" + topic;
    }

    DomainParticipant* participant_;
    FastDDSSubscriber* fastdds_subscriber_;
    TypeSupport type_support_;
    std::function<void(const std::string&, const std::vector<uint8_t>&)> callback_;

    struct ReaderData {
        DataReader* reader = nullptr;
        Topic* topic = nullptr;
        PetrelDataReaderListener* listener = nullptr;
    };

    std::map<std::string, ReaderData> readers_;
};

// Publisher implementation
Publisher::Publisher() : pimpl_(std::make_unique<Impl>()) {}
Publisher::~Publisher() = default;

bool Publisher::init() {
    return pimpl_->init();
}

bool Publisher::publish(const std::string& topic, const std::vector<uint8_t>& data) {
    return pimpl_->publish(topic, data);
}

void Publisher::stop() {
    pimpl_->stop();
}

// Subscriber implementation
Subscriber::Subscriber() : pimpl_(std::make_unique<Impl>()) {}
Subscriber::~Subscriber() = default;

bool Subscriber::init(std::function<void(const std::string&, const std::vector<uint8_t>&)> callback) {
    return pimpl_->init(callback);
}

bool Subscriber::subscribe(const std::string& topic) {
    return pimpl_->subscribe(topic);
}

void Subscriber::stop() {
    pimpl_->stop();
}

} // namespace petrel
