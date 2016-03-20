#pragma once

#include <assert.h>
#include <string>
#include <unordered_map>

#include "uv.h"

#include "conn_base.h"
#include "proto/message.pb.h"

namespace bchat {
namespace chat {

class UserRepo;
class ServerContext;

// User contains the chat's application logic.
//
// Each User is owned by a unique Conn.
//
// It is not safe to call from multiple threads.
//
class User {
 private:
  typedef proto::Error::Type ErrType;

 public:
  User(Sender& sender, ServerContext& server_context);

  // OnMessage is called when the user has sent a message to the server
  void OnMessage(uv_buf_t msg);

  void OnConnect();

  // OnDisconnect is called when the user has disconnected from the server
  void OnDisconnect();

 private:
  void Authenticate(const proto::Authentication& auth);

  void SendResponse();

  void SendError(ErrType);

  void SendMessage(const proto::Message& msg);

 private:
  Sender& sender_;

  bool authenticated_ = false;

  std::string name_;

  ServerContext& sc_;

  // Reused for fewer memory allocations
  proto::Request req_;

  // Reused for fewer memory allocations
  proto::Response res_;
};
}
}
