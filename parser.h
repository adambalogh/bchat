#pragma once

#include <assert.h>
#include <array>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <sstream>
#include <vector>
#include <inttypes.h>

const size_t LENGTH_SIZE = 4;

// Parser provides a convenient interface for reading messages that are prefixed
// with their length in bytes.
//
// Example:
//
//   Parser p;
//   p.Sink(...);
//   if (p.HasMessages()) {
//     auto messages = p.GetMessages();
//     for (auto msg : messages){
//       ...
//     }
//   }
//
class Parser {
 private:
  enum class State { HEADER, BODY };

  typedef std::vector<uint8_t> Message;

 public:
  size_t Fill(uint8_t* dest, size_t dest_size, uint8_t* buf, size_t buf_size) {
    auto read = std::min(buf_size, dest_size);
    assert(read >= 0);
    std::memcpy(dest, buf, read);
    return read;
  }

  // Sink parses and processes the given buffer, it does not take ownership of
  // the buffer
  void Sink(uint8_t* const buf, const size_t size) {
    size_t buf_end = 0;

    while (buf_end < size) {
      if (state_ == State::HEADER) {
        auto read = Fill(length_buf_.data() + length_buf_end_,
                         length_buf_.size() - length_buf_end_, buf + buf_end,
                         size - buf_end);
        length_buf_end_ += read;
        buf_end += read;

        if (length_buf_end_ == LENGTH_SIZE) {
          msg_buf_.reset(new std::vector<uint8_t>(ParseSize()));
          length_buf_end_ = 0;
          msg_buf_end_ = 0;
          state_ = State::BODY;
        }
      } else if (state_ == State::BODY) {
        auto read = Fill(msg_buf_->data() + msg_buf_end_,
                         msg_buf_->size() - msg_buf_end_, buf + buf_end,
                         size - buf_end);
        msg_buf_end_ += read;
        buf_end += read;

        if (msg_buf_end_ == msg_buf_->size()) {
          messages_.push_back(std::move(msg_buf_));
          state_ = State::HEADER;
        }
      }
    }
    assert(buf_end == size);
  }

  // Returns true if there are any messages to return
  bool HasMessages() const { return messages_.size() > 0; }

  // Returns the next available message.
  // It should only be called if HasMessages returns true.
  std::vector<std::shared_ptr<Message>> GetMessages() {
    assert(HasMessages() == true);

    auto out = std::move(messages_);
    messages_.clear();
    return std::move(out);
  }

 private:
  // Converts length_buf to size_t
  size_t ParseSize() {
    assert(length_buf_end_ == LENGTH_SIZE);

    size_t size = 0;
    for (int i = 0; i < LENGTH_SIZE; ++i) {
      size += (length_buf_[i] << (8 * (LENGTH_SIZE - i - 1)));
    }
    return size;
  }

 private:
  State state_ = State::HEADER;

  std::array<uint8_t, LENGTH_SIZE> length_buf_;
  size_t length_buf_end_ = 0;

  std::shared_ptr<std::vector<uint8_t>> msg_buf_;
  size_t msg_buf_end_ = 0;

  std::vector<std::shared_ptr<Message>> messages_;
};
