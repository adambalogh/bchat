#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "chat/user.h"
#include "chat/server_context.h"
#include "../proto/message.pb.h"
#include "../test_util.h"

using namespace ::testing;
using namespace bchat::chat;

TEST(ServerContext, Empty) {
  ServerContext sc;
  EXPECT_FALSE(sc.online_users().Contains(""));
  EXPECT_FALSE(sc.online_users().Contains("a"));
  EXPECT_NO_THROW(sc.online_users().Remove(""));
}

TEST(User, InvalidRequest) {
  MockSender sender;
  ServerContext sc;
  User u{sender, sc};

  proto::Response res;
  res.set_type(res.Error);
  res.mutable_error()->set_type(proto::Error_Type_InvalidRequest);
  auto bin = MakeMsg(res.SerializeAsString());

  EXPECT_CALL(sender, SendProxy(Pointee(*bin)));

  u.OnMessage(MakeBuf("\n"));
}

TEST(User, MustAuthenticateFirst) {
  MockSender sender;
  ServerContext sc;
  User u{sender, sc};

  proto::Response res;
  res.set_type(res.Error);
  res.mutable_error()->set_type(proto::Error_Type_MustAuthenticateFirst);
  auto bin = MakeMsg(res.SerializeAsString());

  EXPECT_CALL(sender, SendProxy(Pointee(*bin)));

  proto::Request req;
  req.set_type(req.Message);
  auto serialized = req.SerializeAsString();
  u.OnMessage(MakeBuf(serialized));
}

TEST(User, UsernameTaken) {
  ServerContext sc;

  // Register 1st User
  MockSender sender;
  User u{sender, sc};

  proto::Request req;
  req.set_type(req.Authentication);
  req.mutable_authentication()->set_name("Adam");
  auto serialized = req.SerializeAsString();
  u.OnMessage(MakeBuf(serialized));

  // Register 2nd user with same name
  MockSender sender2;
  User u2{sender2, sc};

  proto::Response res;
  res.set_type(res.Error);
  res.mutable_error()->set_type(proto::Error_Type_UsernameTaken);
  auto bin = MakeMsg(res.SerializeAsString());

  EXPECT_CALL(sender2, SendProxy(Pointee(*bin)));

  req.Clear();
  req.set_type(req.Authentication);
  req.mutable_authentication()->set_name("Adam");
  serialized = req.SerializeAsString();
  u2.OnMessage(MakeBuf(serialized));
}

TEST(User, DiscconnectsUser) {
  ServerContext sc;

  proto::Request req;
  req.set_type(req.Authentication);
  req.mutable_authentication()->set_name("Adam");

  // Register 1st User
  MockSender sender;
  User u{sender, sc};
  auto serialized = req.SerializeAsString();
  u.OnMessage(MakeBuf(serialized));

  u.OnDisconnect();

  // Register 2nd user with same name
  MockSender sender2;
  User u2{sender2, sc};

  EXPECT_CALL(sender2, SendProxy(_)).Times(0);

  req.Clear();
  req.set_type(req.Authentication);
  req.mutable_authentication()->set_name("Adam");

  serialized = req.SerializeAsString();
  u2.OnMessage(MakeBuf(serialized));
}

TEST(User, MessageSend) {
  MockSender sender;
  ServerContext sc;
  User u{sender, sc};

  proto::Request req;
  req.set_type(req.Authentication);
  req.mutable_authentication()->set_name("Adam");
  auto serialized = req.SerializeAsString();
  u.OnMessage(MakeBuf(serialized));

  proto::Message message;
  message.set_content("Hello!");
  message.set_recipient("Adam");

  proto::Response res;
  res.set_type(res.Message);
  *res.mutable_message() = message;
  res.mutable_message()->set_sender("Adam");
  auto bin = MakeMsg(res.SerializeAsString());

  EXPECT_CALL(sender, SendProxy(Pointee(*bin)));

  req.Clear();

  req.set_type(req.Message);
  *req.mutable_message() = message;
  serialized = req.SerializeAsString();
  u.OnMessage(MakeBuf(serialized));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
