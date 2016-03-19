#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "message_storage.h"
#include "proto/message.pb.h"

using namespace ::testing;

bool operator==(const proto::Message& a, const proto::Message& b) {
  return a.DebugString() == b.DebugString();
}

TEST(MemoryMessageStorage, GetMessagesFrom) {
  MemoryMessageStorage s;

  proto::Message alice_to_bob;
  alice_to_bob.set_sender("Alice");
  alice_to_bob.set_recipient("Bob");
  alice_to_bob.set_content("Hello!");

  proto::Message bob_to_alice;
  bob_to_alice.set_sender("Bob");
  bob_to_alice.set_recipient("Alice");
  bob_to_alice.set_content("Hi!");

  proto::Message alice_to_bob2;
  alice_to_bob2.set_sender("Alice");
  alice_to_bob2.set_recipient("Bob");
  alice_to_bob2.set_content("How are you?");

  proto::Message alice_to_else;
  alice_to_else.set_sender("Alice");
  alice_to_else.set_recipient("Ethan");
  alice_to_else.set_content("Hey");

  proto::Message else_to_bob;
  else_to_bob.set_sender("Ethan");
  else_to_bob.set_recipient("Bob");
  else_to_bob.set_content("hi bob");

  s.Store(alice_to_bob);
  s.Store(bob_to_alice);
  s.Store(alice_to_bob2);
  s.Store(alice_to_else);
  s.Store(else_to_bob);

  auto res = s.GetMessagesFrom("Bob", "Alice");

  EXPECT_EQ(2, res.size());
  // EXPECT_THAT(res, UnorderedElementsAre(alice_to_bob, alice_to_bob2));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
