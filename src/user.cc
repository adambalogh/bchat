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
      Authenticate(std::move(bin));
      return;
    } else {
      SendErrorMsg("You are not authenticated");
      return;
    }
  } else if (req.type() == req.Authentication) {
    SendErrorMsg("You are already authenticated");
    return;
  }

  assert(authenticated_ == true);
  assert(req.type() != req.Authentication);

  if (req.type() == req.Message) {
    proto::Message message;
    auto rc = message.ParseFromArray(bin->data(), bin->size());
    assert(rc == true);
    auto recipient = user_repo_.Get(message.recipient());
    message.set_sender(name_);
    recipient.SendMessage(message);
  } else if (req.type() == req.MessagesList) {
    // return messages
  }
}

void User::Authenticate(MessagePtr msg) {
  proto::Authentication auth;
  if (!auth.ParseFromArray(msg->data(), msg->size())) {
    SendErrorMsg("Invalid authentication");
    return;
  }
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
  proto::Request req;
  req.set_type(req.Error);
  req.set_body(error_msg);

  auto bin = std::make_unique<Conn::Message>(req.ByteSize());
  req.SerializeToArray(bin->data(), bin->size());
  conn_.Send(std::move(bin));
}

void User::SendMessage(proto::Message msg) {
  proto::Request req;
  req.set_type(req.Message);
  req.set_body(std::move(msg.SerializeAsString()));

  auto bin = std::make_unique<Conn::Message>(req.ByteSize());
  req.SerializeToArray(bin->data(), bin->size());
  conn_.Send(std::move(bin));
}

void User::OnDisconnect() { user_repo_.Remove(name_); }
