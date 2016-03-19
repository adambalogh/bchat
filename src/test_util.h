#pragma once

#include <assert.h>
#include <string>

#include <gmock/gmock.h>
#include <benchmark/benchmark.h>

#include "sender.h"

bchat::MessagePtr MakeMsg(std::string s) {
  auto msg = std::make_unique<bchat::Message>(s.size());
  std::copy(s.begin(), s.end(), msg->begin());
  return std::move(msg);
}

uv_buf_t MakeBuf(const std::string& s) {
  uv_buf_t buf;
  buf.len = s.size();
  buf.base = (char*)s.data();
  return buf;
}

typedef std::vector<uint8_t> Arr;

void SetBuf(uv_buf_t dest, Arr values) {
  assert(dest.len >= values.size());
  std::copy(values.begin(), values.end(), dest.base);
}

void SetBuf(uv_buf_t dest, uint8_t* values, size_t size) {
  assert(dest.len >= size);
  std::copy(values, values + size, dest.base);
}

class EmptySender : public bchat::Sender {
 public:
  void Send(bchat::MessagePtr msg) { benchmark::DoNotOptimize(msg); }
};

class MockSender : public bchat::Sender {
 public:
  // GMock cannot handle non-copyable parameters
  void Send(bchat::MessagePtr msg) { SendProxy(msg.get()); }

  MOCK_METHOD1(SendProxy, void(const bchat::Message*));
};
