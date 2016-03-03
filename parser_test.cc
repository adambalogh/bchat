#include <gtest/gtest.h>

#include "parser.h"

TEST(Parser, Empty) {
  Parser p;
  EXPECT_FALSE(p.HasMessages());
}

TEST(Parser, ZeroMsgLength) {
  uint8_t* buf = new uint8_t[4]{0, 0, 0, 0};

  Parser p;
  p.Sink(buf, 4);

  // Decide what to do in this case
}

TEST(Parser, Length) {
  uint8_t* buf = new uint8_t[4]{0, 0, 0, 10};

  Parser p;
  p.Sink(buf, 4);

  EXPECT_FALSE(p.HasMessages());
}

TEST(Parser, LengthChunks) {
  Parser p;

  p.Sink(new uint8_t{0}, 1);

  EXPECT_FALSE(p.HasMessages());

  uint8_t* buf = new uint8_t[2];
  buf[0] = 0;
  buf[1] = 0;
  p.Sink(buf, 2);

  EXPECT_FALSE(p.HasMessages());

  p.Sink(new uint8_t{1}, 1);
  p.Sink(new uint8_t{10}, 1);

  EXPECT_TRUE(p.HasMessages());
}

TEST(Parser, Message) {
  uint8_t* buf = new uint8_t[6]{0, 0, 0, 2, 1, 6};

  Parser p;
  p.Sink(buf, 6);

  EXPECT_TRUE(p.HasMessages());

  auto msg = std::vector<uint8_t>{1, 6};
  auto messages = p.GetMessages();

  EXPECT_EQ(msg, *messages[0]);
  EXPECT_FALSE(p.HasMessages());
}

TEST(Parser, TwoMessages) {
  uint8_t* buf = new uint8_t[15]{
      0, 0, 0,  3,  // header #2
      5, 3, 10,     // msg #1
      0, 0, 0,  4,  // header #2
      1, 2, 3,  4,  // msg #2
  };

  Parser p;
  p.Sink(buf, 15);

  EXPECT_TRUE(p.HasMessages());

  auto msg = std::vector<uint8_t>{5, 3, 10};
  auto msg2 = std::vector<uint8_t>{1, 2, 3, 4};

  auto messages = p.GetMessages();

  EXPECT_EQ(2, messages.size());
  EXPECT_EQ(msg, *messages[0]);
  EXPECT_EQ(msg2, *messages[1]);
}

TEST(Parser, Split) {
  uint8_t* buf = new uint8_t[3]{0, 0, 0};
  uint8_t* buf2 = new uint8_t[4]{2, 11, 10, 0};
  uint8_t* buf3 = new uint8_t[1]{0};
  uint8_t* buf4 = new uint8_t[5]{0, 4, 1, 2, 3};
  uint8_t* buf5 = new uint8_t[3]{4, 0, 0};

  Parser p;
  p.Sink(buf, 3);
  p.Sink(buf2, 4);

  auto msg1 = std::vector<uint8_t>{11, 10};
  EXPECT_TRUE(p.HasMessages());
  auto messages = p.GetMessages();
  EXPECT_EQ(1, messages.size());
  EXPECT_EQ(msg1, *messages[0]);
  EXPECT_FALSE(p.HasMessages());

  p.Sink(buf3, 1);
  EXPECT_FALSE(p.HasMessages());

  p.Sink(buf4, 5);
  EXPECT_FALSE(p.HasMessages());

  p.Sink(buf5, 3);
  auto msg2 = std::vector<uint8_t>{1, 2, 3, 4};
  EXPECT_TRUE(p.HasMessages());
  messages = p.GetMessages();
  EXPECT_EQ(msg2, *messages[0]);
  EXPECT_FALSE(p.HasMessages());
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
