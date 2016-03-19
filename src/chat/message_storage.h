#pragma once

#include <unordered_map>

#include "proto/message.pb.h"

class MessageStorage {
 public:
  typedef std::vector<proto::Message> MessageList;

  virtual ~MessageStorage() {}

  // Stores a single message
  virtual void Store(const proto::Message& message) = 0;

  // Returns all the messages that have been sent to recipient
  virtual MessageList GetMessages(const std::string& recipient) = 0;

  // Returns all the messages that have been sent to recipient by sender
  virtual MessageList GetMessagesFrom(const std::string& recipient,
                                      const std::string& sender) = 0;
};

class MemoryMessageStorage : public MessageStorage {
 public:
  void Store(const proto::Message& message) {
    assert(message.IsInitialized());
    messages_[message.recipient()][message.sender()].push_back(message);
  }

  MessageList GetMessages(const std::string& recipient) {
    int num_messages = 0;
    for (auto& entry : messages_[recipient]) {
      num_messages += entry.second.size();
    }

    MessageList messages(num_messages);
    int end = 0;
    for (auto& entry : messages_[recipient]) {
      auto& list = entry.second;
      std::copy(messages.begin() + end, list.begin(), list.end());
      end += list.size();
    }
    assert(num_messages == end);
    return std::move(messages);
  }

  MessageList GetMessagesFrom(const std::string& recipient,
                              const std::string& sender) {
    return messages_[recipient][sender];
  }

 private:
  std::unordered_map<std::string, std::unordered_map<std::string, MessageList>>
      messages_;
};
