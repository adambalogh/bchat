#pragma once

#include <assert.h>
#include <unordered_map>

#include "proto/message.pb.h"

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
class User {
 public:
  typedef std::unique_ptr<std::vector<uint8_t>> MessagePtr;

  User(Conn& conn, UserRepo& user_repo);

  void OnMessage(MessagePtr msg);

  void SendMessage(proto::Message msg);

  void OnDisconnect();

 private:
  void Authenticate(const proto::Authentication& auth);

  void SendResponse(const proto::Response& res);

  void SendErrorMsg(const std::string& error_msg);

  Conn& conn_;

  bool authenticated_ = false;

  std::string name_;

  UserRepo& user_repo_;
};
