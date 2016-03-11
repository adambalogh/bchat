#include <gmock/gmock.h>
#include <benchmark/benchmark.h>

#include "conn.h"

Conn::MessagePtr MakeMsg(std::string s) {
  auto msg = std::make_unique<Conn::Message>(s.size());
  std::copy(s.begin(), s.end(), msg->begin());
  return std::move(msg);
}

uv_buf_t MakeBuf(const std::string& s) {
  uv_buf_t buf;
  buf.len = s.size();
  buf.base = (char*)s.data();
  return buf;
}

class EmptyConn : public Conn {
 public:
  virtual void OnRead(ssize_t nread, const uv_buf_t* buf) {}
  virtual void Send(const MessagePtr msg) { benchmark::DoNotOptimize(msg); }
};

class MockConn : public Conn {
 public:
  // GMock cannot handle non-copyable parameters
  void Send(const MessagePtr msg) { SendProxy(msg.get()); }

  MOCK_METHOD2(OnRead, void(ssize_t, const uv_buf_t*));
  MOCK_METHOD1(SendProxy, void(const Message*));
};

typedef std::vector<uint8_t> Arr;

void SetBuf(uv_buf_t dest, Arr values) {
  assert(dest.len >= values.size());
  std::copy(values.begin(), values.end(), dest.base);
}

void SetBuf(uv_buf_t dest, uint8_t* values, size_t size) {
  assert(dest.len >= size);
  std::copy(values, values + size, dest.base);
}
