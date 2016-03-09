#include "user.h"

#include "conn.h"

User::User(Conn& conn, UserRepo& user_repo)
    : name_(std::to_string(std::rand())), conn_(conn), user_repo_(user_repo) {
  user_repo_.RegisterUser(name_, this);
}

void User::OnMessage(Conn::MessagePtr bin) {
  proto::Message message;
  auto rc = message.ParseFromArray(bin->data(), bin->size());
  assert(rc == true);

  auto recipient = user_repo_.GetUser(message.recipient());
  message.set_sender(name_);

  recipient.SendMessage(message);
}

void User::SendMessage(proto::Message msg) {
  auto bin = std::make_unique<Conn::Message>(msg.ByteSize());
  msg.SerializeToArray(bin->data(), bin->size());
  conn_.Send(std::move(bin));
}

void User::OnDisconnect() { user_repo_.RemoveUser(name_); }
