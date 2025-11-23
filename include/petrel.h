#ifndef PETREL_HPP
#define PETREL_HPP

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

namespace fastdds {
using namespace eprosima::fastdds::dds;
}

namespace petrel {

template <typename T> struct Message {
    using type = void;
};

template <typename T> class TypedDataReaderListener : public fastdds::DataReaderListener {
  public:
    TypedDataReaderListener(std::function<void(const std::string &, const T &)> callback,
                            const std::string &topic);
    void on_data_available(fastdds::DataReader *reader) override;

  private:
    std::function<void(const std::string &, const T &)> callback_;
    std::string topic_;
};

template <typename T> class Publisher {
  public:
    Publisher();
    ~Publisher();

    bool init(int domain_id = 0);
    bool publish(const std::string &topic_name, const T &message);
    void stop();

  private:
    struct WriterData {
        fastdds::DataWriter *writer = nullptr;
        fastdds::Topic *topic = nullptr;
    };

    fastdds::DomainParticipant *participant_;
    fastdds::Publisher *publisher_;
    fastdds::TypeSupport type_;
    std::mutex mutex_;
    std::map<std::string, WriterData> writers_;
};

template <typename T> class Subscriber {
  public:
    Subscriber();
    ~Subscriber();

    bool init(std::function<void(const std::string &, const T &)> callback, int domain_id = 0);
    bool subscribe(const std::string &topic_name);
    void stop();

  private:
    struct ReaderData {
        fastdds::DataReader *reader = nullptr;
        fastdds::Topic *topic = nullptr;
        TypedDataReaderListener<T> *listener = nullptr;
    };

    fastdds::DomainParticipant *participant_;
    fastdds::Subscriber *subscriber_;
    fastdds::TypeSupport type_;
    std::function<void(const std::string &, const T &)> callback_;
    std::mutex mutex_;
    std::map<std::string, ReaderData> readers_;
};

template <typename T>
TypedDataReaderListener<T>::TypedDataReaderListener(
    std::function<void(const std::string &, const T &)> callback, const std::string &topic)
    : callback_(callback), topic_(topic) {}

template <typename T>
void TypedDataReaderListener<T>::on_data_available(fastdds::DataReader *reader) {
    fastdds::SampleInfo info;
    T data;
    while (reader->take_next_sample(&data, &info) == fastdds::RETCODE_OK) {
        if (info.valid_data) {
            callback_(topic_, data);
        }
    }
}

template <typename T>
Publisher<T>::Publisher()
    : participant_(nullptr), publisher_(nullptr), type_(new typename Message<T>::type()) {}

template <typename T> Publisher<T>::~Publisher() { stop(); }

template <typename T> bool Publisher<T>::init(int domain_id) {
    if (participant_ != nullptr) {
        return false;
    }

    participant_ = fastdds::DomainParticipantFactory::get_instance()->create_participant(
        domain_id, fastdds::PARTICIPANT_QOS_DEFAULT);
    if (participant_ == nullptr) {
        return false;
    }

    type_.register_type(participant_);

    publisher_ = participant_->create_publisher(fastdds::PUBLISHER_QOS_DEFAULT);
    if (publisher_ == nullptr) {
        return false;
    }

    return true;
}

template <typename T> bool Publisher<T>::publish(const std::string &topic_name, const T &message) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (participant_ == nullptr || publisher_ == nullptr) {
        return false;
    }

    auto it = writers_.find(topic_name);
    if (it == writers_.end()) {
        fastdds::Topic *topic = participant_->create_topic(topic_name, type_.get_type_name(),
                                                           fastdds::TOPIC_QOS_DEFAULT);
        if (topic == nullptr) {
            return false;
        }

        fastdds::DataWriter *writer =
            publisher_->create_datawriter(topic, fastdds::DATAWRITER_QOS_DEFAULT);
        if (writer == nullptr) {
            participant_->delete_topic(topic);
            return false;
        }

        writers_[topic_name] = {writer, topic};
        it = writers_.find(topic_name);
    }

    return it->second.writer->write((void *)&message) == fastdds::RETCODE_OK;
}

template <typename T> void Publisher<T>::stop() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (publisher_ != nullptr) {
        for (auto &pair : writers_) {
            if (pair.second.writer != nullptr) {
                publisher_->delete_datawriter(pair.second.writer);
            }
        }
    }

    if (participant_ != nullptr) {
        for (auto &pair : writers_) {
            if (pair.second.topic != nullptr) {
                participant_->delete_topic(pair.second.topic);
            }
        }

        if (publisher_ != nullptr) {
            participant_->delete_publisher(publisher_);
            publisher_ = nullptr;
        }

        fastdds::DomainParticipantFactory::get_instance()->delete_participant(participant_);
        participant_ = nullptr;
    }

    writers_.clear();
}

template <typename T>
Subscriber<T>::Subscriber()
    : participant_(nullptr), subscriber_(nullptr), type_(new typename Message<T>::type()) {}

template <typename T> Subscriber<T>::~Subscriber() { stop(); }

template <typename T>
bool Subscriber<T>::init(std::function<void(const std::string &, const T &)> callback,
                         int domain_id) {
    if (participant_ != nullptr) {
        return false;
    }

    callback_ = callback;

    participant_ = fastdds::DomainParticipantFactory::get_instance()->create_participant(
        domain_id, fastdds::PARTICIPANT_QOS_DEFAULT);
    if (participant_ == nullptr) {
        return false;
    }

    type_.register_type(participant_);

    subscriber_ = participant_->create_subscriber(fastdds::SUBSCRIBER_QOS_DEFAULT);
    if (subscriber_ == nullptr) {
        return false;
    }

    return true;
}

template <typename T> bool Subscriber<T>::subscribe(const std::string &topic_name) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (participant_ == nullptr || subscriber_ == nullptr) {
        return false;
    }

    if (readers_.find(topic_name) != readers_.end()) {
        return false;
    }

    fastdds::Topic *topic =
        participant_->create_topic(topic_name, type_.get_type_name(), fastdds::TOPIC_QOS_DEFAULT);
    if (topic == nullptr) {
        return false;
    }

    auto *listener = new TypedDataReaderListener<T>(callback_, topic_name);

    fastdds::DataReader *reader =
        subscriber_->create_datareader(topic, fastdds::DATAREADER_QOS_DEFAULT, listener);
    if (reader == nullptr) {
        delete listener;
        participant_->delete_topic(topic);
        return false;
    }

    readers_[topic_name] = {reader, topic, listener};
    return true;
}

template <typename T> void Subscriber<T>::stop() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (subscriber_ != nullptr) {
        for (auto &pair : readers_) {
            if (pair.second.reader != nullptr) {
                subscriber_->delete_datareader(pair.second.reader);
            }
            if (pair.second.listener != nullptr) {
                delete pair.second.listener;
            }
        }
    }

    if (participant_ != nullptr) {
        for (auto &pair : readers_) {
            if (pair.second.topic != nullptr) {
                participant_->delete_topic(pair.second.topic);
            }
        }

        if (subscriber_ != nullptr) {
            participant_->delete_subscriber(subscriber_);
            subscriber_ = nullptr;
        }

        fastdds::DomainParticipantFactory::get_instance()->delete_participant(participant_);
        participant_ = nullptr;
    }

    readers_.clear();
}

} // namespace petrel

#endif // PETREL_HPP
