#pragma once

#include <assert.h>
#include <array>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <vector>
#include <inttypes.h>

const size_t LENGTH_SIZE = 4;

struct Array {
  Array(uint8_t* const buf, size_t original_size, size_t start = 0)
      : buf_(buf), original_size_(original_size), start_(start) {
    assert(start_ < original_size_);
  }

  void Delete() { delete[] buf_; }

  uint8_t* start() { return buf_ + start_; }
  void add_start(size_t start) {
    start_ += start;
    assert(size() >= 0);
    if (start_ == original_size_) {
      valid_ = false;
    }
  }

  size_t size() { return original_size_ - start_; }

  bool valid() { return valid_; }

  std::string to_string() {
    std::stringstream repr;
    repr << "original size: " << original_size_ << std::endl;
    repr << "start: " << start_ << std::endl;
    return repr.str();
  }

 private:
  uint8_t* buf_;
  size_t original_size_;
  size_t start_;

  bool valid_ = true;
};

// This class is not thread safe
//
class Parser {
 public:
  ~Parser() {
    for (auto& b : buffers_) {
      b.Delete();
    }
  }

  void Sink(uint8_t* const buf, const size_t size) { AddBuffer({buf, size}); }

  bool HasNext() {
    if (state_ == State::HEADER) {
      if (!TryParseHeader()) {
        return false;
      }
    }
    return next_msg_length_ <= buffers_total_length_;
  }

  std::vector<uint8_t> GetNext() {
    assert(state_ == State::BODY && HasNext() == true);
    std::vector<uint8_t> msg(next_msg_length_);
    size_t msg_end = 0;

    for (auto& buf : buffers_) {
      if (!buf.valid()) {
        continue;
      }

      size_t needed = std::min(buf.size(), next_msg_length_ - msg_end);
      std::memcpy(msg.data() + msg_end, buf.start(), needed);
      msg_end += needed;
      buf.add_start(needed);
      buffers_total_length_ -= needed;

      if (msg_end == next_msg_length_) {
        break;
      }
    }
    assert(msg_end == next_msg_length_);

    FreeBuffers();
    state_ = State::HEADER;
    return std::move(msg);
  }

 private:
  bool TryParseHeader() {
    if (LENGTH_SIZE > buffers_total_length_) {
      return false;
    }

    std::array<uint8_t, LENGTH_SIZE> header;
    size_t header_end = 0;

    for (int i = 0; i < buffers_.size(); ++i) {
      auto& buf = buffers_[i];
      assert(buf.valid() == true);

      auto read = std::min(buf.size(), LENGTH_SIZE - header_end);
      std::memcpy(header.data() + header_end, buf.start(), read);
      header_end += read;
      buf.add_start(read);
      buffers_total_length_ -= read;

      assert(header_end <= LENGTH_SIZE);

      if (header_end == LENGTH_SIZE) {
        next_msg_length_ = ParseSize(header.data());
        state_ = State::BODY;
        return true;
      }
    }

    return false;
  }

  void FreeBuffers() {
    int valid_buf_index = 0;
    for (int i = 0; i < buffers_.size(); ++i) {
      if (!buffers_[i].valid()) {
        valid_buf_index = i + 1;
        buffers_[i].Delete();
      } else {
        valid_buf_index = i;
        break;
      }
    }

    buffers_.erase(buffers_.begin(), buffers_.begin() + valid_buf_index);
  }

  size_t ParseSize(uint8_t* header) {
    size_t size = 0;
    for (int i = 0; i < LENGTH_SIZE; ++i) {
      size += (header[i] << (8 * (LENGTH_SIZE - i - 1)));
    }
    return size;
  }

  void AddBuffer(Array a) {
    buffers_total_length_ += a.size();
    buffers_.push_back(a);
  }

  enum class State { HEADER, BODY };

  State state_ = State::HEADER;

  std::vector<Array> buffers_;
  size_t buffers_total_length_ = 0;

  size_t next_msg_length_;
};
