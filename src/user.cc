#include "user.h"

#include "conn.h"

User::User(Conn& conn, UserRepo& user_repo)
    : conn_(conn), user_repo_(user_repo) {}

void User::OnMessage(MessagePtr bin) {
  if (!authenticated_) {
    Authenticate(std::move(bin));
    return;
  }

  proto::Message message;
  auto rc = message.ParseFromArray(bin->data(), bin->size());
  assert(rc == true);

  auto recipient = user_repo_.Get(message.recipient());
  message.set_sender(name_);
  recipient.SendMessage(message);
}

void User::Authenticate(MessagePtr msg) {
  proto::Authentication auth;
  if (!auth.ParseFromArray(msg->data(), msg->size())) {
    // error
    return;
  }
  if (!user_repo_.Register(auth.name(), this)) {
    // error
    return;
  }
  name_ = auth.name();
  authenticated_ = true;
}

void User::SendMessage(proto::Message msg) {
  auto bin = std::make_unique<Conn::Message>(msg.ByteSize());
  msg.SerializeToArray(bin->data(), bin->size());
  conn_.Send(std::move(bin));
}

void User::OnDisconnect() { user_repo_.Remove(name_); }
