#pragma once

#include <assert.h>
#include <array>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <sstream>
#include <vector>
#include <inttypes.h>

#include "uv.h"

const size_t LENGTH_SIZE = 4;
const size_t MIN_FREE_SPACE = 1000;

// Parser provides an efficient and convenient way of reading messages
// that are prefixed with their length in bytes.
//
// The layout of the parser's buffer is as follows:
//
//
//   |              buf_size                 |                     |
//   |                                       |                     |
//   | buf_parsed_size |    ParsableSize()   |      FreeSpace()    |
//   |                 |                     |                     |
//   --------------------------------------------------------------
//   | already parsed  | has not been parsed | has not been filled |
//   --------------------------------------------------------------
//                Parsable()             FreeBuf()
//
class Parser {
 private:
  enum class State { HEADER, BODY };

 public:
  typedef std::vector<uint8_t> Message;
  typedef std::unique_ptr<Message> MessagePtr;
  typedef std::function<void(uv_buf_t)> Handle;

  uv_buf_t GetBuf() {
    uv_buf_t buf;
    buf.base = reinterpret_cast<char*>(FreeBuf());
    buf.len = FreeBufSize();
    return buf;
  }

  // Sink parses and processes the given buffer, it does not take ownership of
  // the buffer
  void Sink(const size_t size, Handle handle) {
    assert(buf_size_ + size <= buf_.size());
    assert(size >= 0);
    buf_size_ += size;

    assert(ParsableSize() >= size);
    assert(buf_size_ + FreeBufSize() == buf_.size());
    assert(buf_parsed_size_ + ParsableSize() == buf_size_);

    while (ParsableSize() > 0) {
      if (state_ == State::HEADER) {
        if (ParsableSize() < LENGTH_SIZE) {
          break;
        } else {
          next_size_ = DecodeSize(Parsable());
          Advance(LENGTH_SIZE);
          state_ = State::BODY;
        }
      } else if (state_ == State::BODY) {
        // If we have the whole msg in buffer, handle it
        if (ParsableSize() >= next_size_) {
          uv_buf_t buf;
          buf.base = reinterpret_cast<char*>(Parsable());
          buf.len = next_size_;
          handle(buf);
          Advance(next_size_);
          state_ = State::HEADER;
        } else {
          // If there is enough free space for the rest of the msg,
          // break out of the loop and wait for it
          if (FreeBufSize() >= next_size_) {
            break;
          }
          // If there is not enough space for the next message, even if we
          // clear the buffer, we need to use create a sufficient buffer for
          // the next msg, this shouldn't happen very often
          if (buf_.size() < next_size_) {
            // make large buf
            printf("This should not happen\n");
          }
          // If the next msg can fit in the buffer, free up the part of the
          // buffer that has already been parsed
          else {
            DeleteUsed();
            break;
          }
        }
      }
    }

    assert(buf_size_ + FreeBufSize() == buf_.size());
    assert(buf_parsed_size_ + ParsableSize() == buf_size_);

    if (FreeBufSize() < MIN_FREE_SPACE) {
      DeleteUsed();
    }
  }

 private:
  void DeleteUsed() {
    std::copy(Parsable(), FreeBuf(), buf_.begin());
    buf_size_ = ParsableSize();
    buf_parsed_size_ = 0;
  }

  void Advance(size_t size) {
    buf_parsed_size_ += size;
    assert(buf_parsed_size_ <= buf_size_);
  }

  uint8_t* Parsable() { return buf_.data() + buf_parsed_size_; }
  const uint8_t* Parsable() const { return buf_.data() + buf_parsed_size_; }
  size_t ParsableSize() const { return buf_size_ - buf_parsed_size_; }

  uint8_t* FreeBuf() { return buf_.data() + buf_size_; }
  const uint8_t* FreeBuf() const { return buf_.data() + buf_size_; }
  size_t FreeBufSize() const { return buf_.size() - buf_size_; }

  // Converts length_buf to size_t
  size_t DecodeSize(const uint8_t* buf) {
    size_t size = 0;
    for (int i = 0; i < LENGTH_SIZE; ++i) {
      size += (buf[i] << (8 * (LENGTH_SIZE - i - 1)));
    }
    return size;
  }

 private:
  State state_ = State::HEADER;

  // We assume that the majority of the messages will be shorter than 10KB, so
  // they all fit in this buffer, in case they do not ...
  std::array<uint8_t, 10000> buf_;

  size_t buf_parsed_size_ = 0;
  size_t buf_size_ = 0;

  size_t next_size_ = 0;
};
