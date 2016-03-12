#pragma once

#include <assert.h>
#include <unordered_map>

#include "proto/message.pb.h"

#include "uv.h"

class User;
class Conn;

class UserRepo {
 public:
  bool Register(const std::string& name, User* user) {
    assert(user != nullptr);
    if (Contains(name)) {
      return false;
    }
    users_[name] = user;
    return true;
  }

  User& Get(const std::string& name) { return *users_.at(name); }

  bool Contains(const std::string& name) const {
    return users_.count(name) == 1;
  }

  void Remove(const std::string& name) {
    auto user = users_.find(name);
    if (user != users_.end()) {
      users_.erase(user);
    }
  }

 private:
  std::unordered_map<std::string, User*> users_;
};

// User contains the chat's application logic.
//
// Each User is owned by a unique Conn.
//
// It is not safe to call from multiple threads.
//
class User {
 public:
 private:
  typedef proto::Error::Type ErrType;

 public:
  User(Conn& conn, UserRepo& user_repo);

  // OnMessage is called when the user has sent a message to the server
  void OnMessage(uv_buf_t msg);

  // OnDisconnect is called when the user has disconnected from the server
  void OnDisconnect();

 private:
  void Authenticate(const proto::Authentication& auth);

  void SendResponse();

  void SendError(ErrType);

  void SendMessage(const proto::Message& msg);

 private:
  Conn& conn_;

  bool authenticated_ = false;

  std::string name_;

  UserRepo& user_repo_;

  // Reused for fewer memory allocations
  proto::Request req_;

  // Reused for fewer memory allocations
  proto::Response res_;
};
