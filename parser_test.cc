#include <gtest/gtest.h>

#include "parser.h"

TEST(Parser, ZeroMsgLength) {
  uint8_t* buf = new uint8_t[4];
  buf[0] = 0;
  buf[1] = 0;
  buf[2] = 0;
  buf[3] = 0;

  Parser p;
  p.Sink(buf, 4);

  EXPECT_TRUE(p.HasNext());
}

TEST(Parser, Length) {
  uint8_t* buf = new uint8_t[4];
  buf[0] = 0;
  buf[1] = 0;
  buf[2] = 0;
  buf[3] = 10;

  Parser p;
  p.Sink(buf, 4);

  EXPECT_FALSE(p.HasNext());
}

TEST(Parser, LengthChunks) {
  Parser p;

  p.Sink(new uint8_t{0}, 1);

  EXPECT_FALSE(p.HasNext());

  uint8_t* buf = new uint8_t[2];
  buf[0] = 0;
  buf[1] = 0;
  p.Sink(buf, 2);

  EXPECT_FALSE(p.HasNext());

  p.Sink(new uint8_t{0}, 1);

  EXPECT_TRUE(p.HasNext());
}

TEST(Parser, Message) {
  uint8_t* buf = new uint8_t[6];
  buf[0] = 0;
  buf[1] = 0;
  buf[2] = 0;
  buf[3] = 2;
  buf[4] = 1;
  buf[5] = 6;

  Parser p;
  p.Sink(buf, 6);

  EXPECT_TRUE(p.HasNext());
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
