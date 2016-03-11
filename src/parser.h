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
// Example usage:
//
//   Parser p;
//   auto written = FillBuf(p.GetBuf()); // e.g. from network
//   p.Sink(written, ...);
//
class Parser {
 private:
  enum class State { HEADER, BODY };

 public:
  typedef std::function<void(uv_buf_t)> Handle;

  // GetBuf returns a buffer that should be filled with the data we want to
  // parse. The size of the data copied into this buffer must be less than
  // buf.len.
  uv_buf_t GetBuf() {
    uv_buf_t buf;
    buf.base = reinterpret_cast<char*>(FreeBuf());
    buf.len = FreeBufSize();
    return buf;
  }

  // Sink parses the given buffer, and calls handle for each message that is
  // fully available.
  //
  // It does not take ownership of the given buffer.
  void Sink(const size_t size, Handle handle) {
    assert(size >= 0);
    assert(buf_size_ + size <= buf_.size());

    buf_size_ += size;
    assert(ParsableSize() >= size);

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
          // break out of the loop and wait for it to be Sinked by the user
          if (FreeBufSize() >= next_size_) {
            break;
          }
          // If there is not enough space for the next message, even if we
          // clear the buffer, we need to create a sufficiently large buffer for
          // the next msg, this shouldn't happen very often
          if (buf_.size() < next_size_) {
            // TODO handle this case
            printf("This should not happen\n");
            exit(1);
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

    // Try to make sure that there is always at leat MIN_FREE_SPACE free space
    // in the buffer. This may not always be possible, e.g. if the size of the
    // next message is close to the maximum size of the buffer.
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
  // they all fit in this buffer, in case they do not we need to allocate an
  // extra buffer only for that message.
  std::array<uint8_t, 10000> buf_;

  // See class comments for explanation
  size_t buf_parsed_size_ = 0;
  size_t buf_size_ = 0;

  // The size of the next message we are expecting, in bytes
  size_t next_size_ = 0;
};
