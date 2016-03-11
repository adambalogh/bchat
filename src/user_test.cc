#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "user.h"
#include "conn.h"
#include "proto/message.pb.h"

using namespace ::testing;

const std::string name = "Adam";

class MockConn : public Conn {
 public:
  // GMock cannot handle non-copyable parameters
  void Send(const MessagePtr msg) { SendProxy(msg.get()); }

  MOCK_METHOD2(OnRead, void(ssize_t, const uv_buf_t*));
  MOCK_METHOD1(SendProxy, void(const Message*));
};

Conn::MessagePtr MakeMsg(std::string s) {
  auto msg = std::make_unique<Conn::Message>(s.size());
  std::copy(s.begin(), s.end(), msg->begin());
  return std::move(msg);
}

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

  u.OnMessage(MakeMsg("\n"));
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
  u.OnMessage(MakeMsg(req.SerializeAsString()));
}

TEST(User, UsernameTaken) {
  UserRepo repo;

  // Register 1st User
  MockConn conn;
  User u{conn, repo};

  proto::Request req;
  req.set_type(req.Authentication);
  req.mutable_authentication()->set_name("Adam");
  u.OnMessage(MakeMsg(req.SerializeAsString()));

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
  u2.OnMessage(MakeMsg(req.SerializeAsString()));
}

TEST(User, DiscconnectsUser) {
  UserRepo repo;

  proto::Request req;
  req.set_type(req.Authentication);
  req.mutable_authentication()->set_name("Adam");

  // Register 1st User
  MockConn conn;
  User u{conn, repo};
  u.OnMessage(MakeMsg(req.SerializeAsString()));
  u.OnDisconnect();

  // Register 2nd user with same name
  MockConn conn2;
  User u2{conn2, repo};

  EXPECT_CALL(conn2, SendProxy(_)).Times(0);

  req.Clear();
  req.set_type(req.Authentication);
  req.mutable_authentication()->set_name("Adam");
  u2.OnMessage(MakeMsg(req.SerializeAsString()));
}

TEST(User, MessageSend) {
  MockConn conn;
  UserRepo repo;
  User u{conn, repo};

  proto::Request req;
  req.set_type(req.Authentication);
  req.mutable_authentication()->set_name("Adam");
  u.OnMessage(MakeMsg(req.SerializeAsString()));

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
  u.OnMessage(MakeMsg(req.SerializeAsString()));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
