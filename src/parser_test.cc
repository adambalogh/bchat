#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <algorithm>

#include "parser.h"
#include "test_util.h"

#define HANDLE(handle) [&handle](uv_buf_t buf) { handle.Handle(buf); }

using namespace ::testing;

class HandleInterface {
 public:
  virtual ~HandleInterface() {}
  virtual void Handle(uv_buf_t) = 0;
};

class MockHandle : public HandleInterface {
 public:
  MOCK_METHOD1(Handle, void(uv_buf_t));
};

inline bool operator==(const uv_buf_t& a, const uv_buf_t& b) {
  return a.len == b.len && std::equal(a.base, a.base + a.len, b.base);
}

TEST(Parser, Empty) {
  Parser p;
  MockHandle handle;
  p.Sink(0, HANDLE(handle));

  EXPECT_CALL(handle, Handle(_)).Times(0);
}

// TEST(Parser, ZeroMsgLength) {
//  uint8_t buf[4] = {0, 0, 0, 0};

//  Parser p;
//  p.Sink(4, EmptyHandle);

//  // Decide what to expect in this case
//}

TEST(Parser, Length) {
  Parser p;
  Arr buf{0, 0, 0, 10};
  SetBuf(p.GetBuf(), buf);

  MockHandle handle;
  p.Sink(4, HANDLE(handle));

  EXPECT_CALL(handle, Handle(_)).Times(0);
}

TEST(Parser, LengthChunks) {
  Parser p;
  MockHandle handle;

  // 0, 0, 0, 1, 1

  SetBuf(p.GetBuf(), {0});
  p.Sink(1, HANDLE(handle));

  SetBuf(p.GetBuf(), {0, 0});
  p.Sink(2, HANDLE(handle));

  EXPECT_CALL(handle, Handle(_)).Times(1);

  SetBuf(p.GetBuf(), {1, 1});
  p.Sink(2, HANDLE(handle));
}

TEST(Parser, Message) {
  Parser p;
  MockHandle handle;

  uint8_t msg[] = {1, 6};
  uv_buf_t expected;
  expected.base = (char*)msg;
  expected.len = 2;
  EXPECT_CALL(handle, Handle(expected)).Times(1);

  SetBuf(p.GetBuf(), {0, 0, 0, 2, 1, 6});
  p.Sink(6, HANDLE(handle));
}

TEST(Parser, TwoMessages) {
  Parser p;
  MockHandle handle;

  Arr buf{
      0, 0, 0,  3,  // header #2
      5, 3, 10,     // msg #1
      0, 0, 0,  4,  // header #2
      1, 2, 3,  4,  // msg #2
  };

  uint8_t msg1[] = {5, 3, 10};
  uv_buf_t expected1;
  expected1.base = (char*)msg1;
  expected1.len = 3;
  uint8_t msg2[] = {1, 2, 3, 4};
  uv_buf_t expected2;
  expected2.base = (char*)msg2;
  expected2.len = 4;

  {
    InSequence s;
    EXPECT_CALL(handle, Handle(expected1)).Times(1);
    EXPECT_CALL(handle, Handle(expected2)).Times(1);
  }

  SetBuf(p.GetBuf(), buf);
  p.Sink(15, HANDLE(handle));
}

TEST(Parser, Split) {
  Parser p;
  MockHandle handle;

  Arr buf{0, 0, 0};
  Arr buf2{2, 11, 10, 0};
  Arr buf3{0};
  Arr buf4{0, 4, 1, 2, 3};
  Arr buf5{4, 0, 0};

  SetBuf(p.GetBuf(), buf);
  p.Sink(buf.size(), HANDLE(handle));

  uint8_t msg[] = {11, 10};
  uv_buf_t expected;
  expected.base = (char*)msg;
  expected.len = 2;
  EXPECT_CALL(handle, Handle(expected)).Times(1);

  SetBuf(p.GetBuf(), buf2);
  p.Sink(buf2.size(), HANDLE(handle));

  SetBuf(p.GetBuf(), buf3);
  p.Sink(buf3.size(), HANDLE(handle));

  SetBuf(p.GetBuf(), buf4);
  p.Sink(buf4.size(), HANDLE(handle));

  uint8_t msg2[] = {1, 2, 3, 4};
  uv_buf_t expected2;
  expected2.base = (char*)msg2;
  expected2.len = 4;
  EXPECT_CALL(handle, Handle(expected2)).Times(1);

  SetBuf(p.GetBuf(), buf5);
  p.Sink(buf5.size(), HANDLE(handle));
}

TEST(Parser, SeveralMsg) {
  Parser p;
  MockHandle handle;

  Arr length{0, 0, 0, 100};
  Arr msg(100);

  uv_buf_t expected;
  expected.base = (char*)msg.data();
  expected.len = msg.size();

  int rounds = 1000;
  EXPECT_CALL(handle, Handle(expected)).Times(rounds);

  for (int i = 0; i < rounds; ++i) {
    SetBuf(p.GetBuf(), length);
    p.Sink(4, HANDLE(handle));
    SetBuf(p.GetBuf(), msg);
    p.Sink(100, HANDLE(handle));
  }
}

// TEST(Parser, ExtraLargeMsg) {
//  Parser p;
//  MockHandle handle;

//  Arr length{0, 100, 100, 100};
//  Arr msg(1000);

//  SetBuf(p.GetBuf(), length);
//  p.Sink(4, HANDLE(handle));
//  SetBuf(p.GetBuf(), msg);
//  p.Sink(100, HANDLE(handle));
//}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
