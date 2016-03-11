#include "user.h"

#include "proto/message.pb.h"

#include "conn.h"

User::User(Conn& conn, UserRepo& user_repo)
    : conn_(conn), user_repo_(user_repo) {}

void User::OnMessage(MessagePtr bin) {
  proto::Request req;
  if (!req.ParseFromArray(bin->data(), bin->size())) {
    SendErrorMsg("Invalid request");
    return;
  }

  if (!authenticated_) {
    if (req.type() == req.Authentication) {
      assert(req.has_authentication() == true);
      Authenticate(req.authentication());
      return;
    } else {
      SendErrorMsg("You must authenticate yourself first");
      return;
    }
  } else if (req.type() == req.Authentication) {
    SendErrorMsg("You are already authenticated");
    return;
  }

  assert(authenticated_ == true);
  assert(req.type() != req.Authentication);

  if (req.type() == req.Message) {
    assert(req.has_message() == true);
    proto::Message msg = req.message();
    msg.set_sender(name_);
    auto recipient = user_repo_.Get(msg.recipient());
    recipient.SendMessage(msg);
  } else if (req.type() == req.MessagesListReq) {
    // return messages
  }
}

void User::Authenticate(const proto::Authentication& auth) {
  if (user_repo_.Contains(auth.name())) {
    SendErrorMsg("Username already taken");
    return;
  }

  auto rc = user_repo_.Register(auth.name(), this);
  assert(rc == true);
  name_ = auth.name();
  authenticated_ = true;
}

void User::SendErrorMsg(const std::string& error_msg) {
  proto::Response res;
  res.set_type(res.Error);
  res.mutable_error()->set_message(error_msg);
  SendResponse(res);
}

void User::SendMessage(const proto::Message& msg) {
  proto::Response res;
  res.set_type(res.Message);
  res.set_allocated_message(const_cast<proto::Message*>(&msg));
  SendResponse(res);
  res.release_message();
}

void User::SendResponse(const proto::Response& res) {
  auto bin = std::make_unique<Conn::Message>(res.ByteSize());
  res.SerializeToArray(bin->data(), bin->size());
  conn_.Send(std::move(bin));
}

void User::OnDisconnect() { user_repo_.Remove(name_); }
