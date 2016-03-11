#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "user.h"
#include "conn.h"
#include "proto/message.pb.h"
#include "test_util.h"

using namespace ::testing;

TEST(UserRepo, Empty) {
  UserRepo repo;
  EXPECT_FALSE(repo.Contains(""));
  EXPECT_FALSE(repo.Contains("a"));
  EXPECT_NO_THROW(repo.Remove(""));
}

TEST(User, InvalidRequest) {
  MockConn conn;
  UserRepo repo;
  User u{conn, repo};

  proto::Response res;
  res.set_type(res.Error);
  res.mutable_error()->set_type(proto::Error_Type_InvalidRequest);
  auto bin = MakeMsg(res.SerializeAsString());

  EXPECT_CALL(conn, SendProxy(Pointee(*bin)));

  u.OnMessage(MakeBuf("\n"));
}

TEST(User, MustAuthenticateFirst) {
  MockConn conn;
  UserRepo repo;
  User u{conn, repo};

  proto::Response res;
  res.set_type(res.Error);
  res.mutable_error()->set_type(proto::Error_Type_MustAuthenticateFirst);
  auto bin = MakeMsg(res.SerializeAsString());

  EXPECT_CALL(conn, SendProxy(Pointee(*bin)));

  proto::Request req;
  req.set_type(req.Message);
  auto serialized = req.SerializeAsString();
  u.OnMessage(MakeBuf(serialized));
}

TEST(User, UsernameTaken) {
  UserRepo repo;

  // Register 1st User
  MockConn conn;
  User u{conn, repo};

  proto::Request req;
  req.set_type(req.Authentication);
  req.mutable_authentication()->set_name("Adam");
  auto serialized = req.SerializeAsString();
  u.OnMessage(MakeBuf(serialized));

  // Register 2nd user with same name
  MockConn conn2;
  User u2{conn2, repo};

  proto::Response res;
  res.set_type(res.Error);
  res.mutable_error()->set_type(proto::Error_Type_UsernameTaken);
  auto bin = MakeMsg(res.SerializeAsString());

  EXPECT_CALL(conn2, SendProxy(Pointee(*bin)));

  req.Clear();
  req.set_type(req.Authentication);
  req.mutable_authentication()->set_name("Adam");
  serialized = req.SerializeAsString();
  u2.OnMessage(MakeBuf(serialized));
}

TEST(User, DiscconnectsUser) {
  UserRepo repo;

  proto::Request req;
  req.set_type(req.Authentication);
  req.mutable_authentication()->set_name("Adam");

  // Register 1st User
  MockConn conn;
  User u{conn, repo};
  auto serialized = req.SerializeAsString();
  u.OnMessage(MakeBuf(serialized));

  u.OnDisconnect();

  // Register 2nd user with same name
  MockConn conn2;
  User u2{conn2, repo};

  EXPECT_CALL(conn2, SendProxy(_)).Times(0);

  req.Clear();
  req.set_type(req.Authentication);
  req.mutable_authentication()->set_name("Adam");

  serialized = req.SerializeAsString();
  u2.OnMessage(MakeBuf(serialized));
}

TEST(User, MessageSend) {
  MockConn conn;
  UserRepo repo;
  User u{conn, repo};

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

  EXPECT_CALL(conn, SendProxy(Pointee(*bin)));

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
