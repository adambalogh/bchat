#include "user.h"

#include "conn_base.h"
#include "chat/server_context.h"
#include "chat/server_context.h"
#include "proto/message.pb.h"

namespace bchat {
namespace chat {

User::User(Sender& sender, ServerContext& server_context)
    : sender_(sender), sc_(server_context) {}

void User::OnMessage(uv_buf_t bin) {
  req_.Clear();
  if (!req_.ParseFromArray(bin.base, bin.len)) {
    SendError(ErrType::Error_Type_InvalidRequest);
    return;
  }

  if (!authenticated_) {
    if (req_.type() == req_.Authentication) {
      assert(req_.has_authentication() == true);
      Authenticate(req_.authentication());
      return;
    } else {
      SendError(ErrType::Error_Type_MustAuthenticateFirst);
      return;
    }
  } else if (req_.type() == req_.Authentication) {
    SendError(ErrType::Error_Type_AlreadyAuthenticated);
    return;
  }

  assert(authenticated_ == true);
  assert(req_.type() != req_.Authentication);

  if (req_.type() == req_.Message) {
    assert(req_.has_message() == true);
    if (!sc_.online_users().Contains(req_.message().recipient())) {
      SendError(ErrType::Error_Type_UserNotOnline);
      return;
    }

    std::unique_ptr<proto::Message> msg(req_.release_message());
    auto& recipient = sc_.online_users().Get(msg->recipient());
    // Avoid unnecessary memory allocation
    msg->set_allocated_sender(&name_);
    recipient.SendMessage(*msg);
    msg->release_sender();

    assert(msg->has_sender() == false);
  } else if (req_.type() == req_.MessagesListReq) {
    // return messages
  }
}

void User::Authenticate(const proto::Authentication& auth) {
  if (!sc_.online_users().Register(auth.name(), this)) {
    SendError(ErrType::Error_Type_UsernameTaken);
    return;
  }
  name_ = auth.name();
  authenticated_ = true;
}

void User::SendError(ErrType type) {
  res_.set_type(res_.Error);
  res_.mutable_error()->set_type(type);
  SendResponse();
}

void User::SendMessage(const proto::Message& msg) {
  res_.set_type(res_.Message);
  res_.set_allocated_message(const_cast<proto::Message*>(&msg));
  SendResponse();
  res_.release_message();
}

// SendResponse sends the Response to the client,
// and resets the instance's Response member, so that it can be reused
void User::SendResponse() {
  auto bin = std::make_unique<Message>(res_.ByteSize());
  res_.SerializeToArray(bin->data(), bin->size());
  sender_.Send(std::move(bin));
  res_.Clear();
}

void User::OnDisconnect() { sc_.online_users().Remove(name_); }

void User::OnConnect() {}
}
}
