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
    if (start_ >= original_size_) {
      throw std::invalid_argument(
          "Invalid start argument, must be less than original_size.");
    }
  }

  // Deletes the underlying buffer
  void Delete() { delete[] buf_; }

  const uint8_t* start() const { return buf_ + start_; }

  void add_start(size_t start) {
    start_ += start;
    assert(size() >= 0);
    if (start_ == original_size_) {
      valid_ = false;
    }
  }

  size_t size() const { return original_size_ - start_; }

  bool valid() const { return valid_; }

  std::string to_string() const {
    std::stringstream repr;
    repr << "buf: " << buf_ << std::endl;
    repr << "original size: " << original_size_ << std::endl;
    repr << "start: " << start_ << std::endl;
    return repr.str();
  }

 private:
  // buf points to the original beginning of the array
  uint8_t* buf_;

  // the size of the original buffer
  size_t original_size_;

  // the offset where users should read from within the buffer
  size_t start_;

  // indicates if this array is valid,
  // it is not valid if there is nothing else to read
  bool valid_ = true;
};

// Parser provides a convenient interface for reading messages that are prefixed
// with their length in bytes.
//
// Example:
//
//   Parser p;
//   p.Sink(...);
//   while (p.HasNext()){
//     auto msg = p.GetNext();
//   }
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

    auto msg = ExtractBytes(next_msg_length_);
    state_ = State::HEADER;
    return std::move(msg);
  }

 private:
  // Extracts and returns n bytes from buffers.
  // It also deletes all buffers that are empty.
  std::vector<uint8_t> ExtractBytes(size_t n) {
    assert(n <= buffers_total_length_);

    std::vector<uint8_t> bytes(n);
    size_t bytes_end = 0;

    for (auto& buf : buffers_) {
      assert(buf.valid() == true);

      auto read = std::min(buf.size(), n - bytes_end);
      std::memcpy(bytes.data() + bytes_end, buf.start(), read);
      bytes_end += read;
      buf.add_start(read);
      buffers_total_length_ -= read;
      if (bytes_end == n) {
        break;
      }
    }

    assert(bytes_end == n);
    FreeBuffers();

    return bytes;
  }

  bool TryParseHeader() {
    if (LENGTH_SIZE > buffers_total_length_) {
      return false;
    }

    auto bytes = ExtractBytes(LENGTH_SIZE);
    next_msg_length_ = ParseSize(bytes.data());
    state_ = State::BODY;
    return true;
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
